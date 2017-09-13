package transaction

import (
	"fmt"
	"sync"

	"github.com/CanonicalLtd/go-sqlite3x"
	"github.com/mattn/go-sqlite3"
	"github.com/ryanfaerman/fsm"
)

// Txn captures information about an active WAL write transaction
// that has been started on a SQLite connection configured to be in
// either leader or replication mode.
type Txn struct {
	conn     *sqlite3.SQLiteConn
	id       string
	isLeader bool
	dryRun   bool
	mu       sync.Mutex // Serialize access to internal state
	entered  bool       // Whether we obtained the mutex
	state    *txnState
	machine  *fsm.Machine
}

// Possible transaction states
const (
	Pending = fsm.State("pending")
	Started = fsm.State("started")
	Writing = fsm.State("writing")
	Undoing = fsm.State("undoing")
	Ended   = fsm.State("ended")
	Stale   = fsm.State("stale")
)

func newTxn(conn *sqlite3.SQLiteConn, id string, isLeader bool, dryRun bool) *Txn {
	state := &txnState{state: Pending}
	machine := &fsm.Machine{
		Subject: state,
		Rules:   newTxnStateRules(isLeader),
	}

	txn := &Txn{
		conn:     conn,
		id:       id,
		isLeader: isLeader,
		dryRun:   dryRun,
		state:    state,
		machine:  machine,
	}

	return txn
}

func (t *Txn) String() string {
	return fmt.Sprintf("{id=%s state=%s leader=%v}", t.id, t.state.CurrentState(), t.isLeader)
}

// Enter starts a critical section accessing or modifying this
// transaction instance.
func (t *Txn) Enter() {
	t.mu.Lock()
	t.entered = true
}

// Exit ends a critical section accessing or modifying this
// transaction instance.
func (t *Txn) Exit() {
	t.entered = false
	t.mu.Unlock()
}

// Conn returns the sqlite connection that started this write
// transaction.
func (t *Txn) Conn() *sqlite3.SQLiteConn {
	return t.conn
}

// ID returns the ID associated with this transaction.
func (t *Txn) ID() string {
	return t.id
}

// State returns the current state of the transition.
func (t *Txn) State() fsm.State {
	t.checkEntered()
	return t.state.CurrentState()
}

// IsLeader returns true if the underlying connection is in leader
// replication mode.
func (t *Txn) IsLeader() bool {
	// This flag is set by the registry at creation time. See Registry.add().
	return t.isLeader
}

// Begin the WAL write transaction.
func (t *Txn) Begin() error {
	t.checkEntered()
	return t.transition(Started)
}

// WalFrames writes frames to the WAL.
func (t *Txn) WalFrames(frames *sqlite3x.ReplicationWalFramesParams) error {
	t.checkEntered()
	return t.transition(Writing, frames)
}

// Undo reverts all changes to the WAL since the start of the
// transaction.
func (t *Txn) Undo() error {
	t.checkEntered()
	return t.transition(Undoing)
}

// End completes the transaction.
func (t *Txn) End() error {
	t.checkEntered()
	return t.transition(Ended)
}

// Stale marks this transaction as stale. It must be called only for
// leader transactions.
func (t *Txn) Stale() error {
	t.checkEntered()
	if t.State() == Started || t.State() == Writing {
		if err := t.transition(Undoing); err != nil {
			return err
		}
	}
	if t.State() == Undoing {
		if err := t.transition(Ended); err != nil {
			return err
		}
	}
	return t.transition(Stale)
}

// Try to transition to the given state. If the transition is invalid,
// panic out.
func (t *Txn) transition(state fsm.State, args ...interface{}) error {
	if err := t.machine.Transition(state); err != nil {
		panic(fmt.Sprintf(
			"invalid %s -> %s transition", t.state.CurrentState(), state))
	}

	if t.dryRun {
		// In dry run mode, don't actually invoke the sqlite APIs.
		return nil
	}

	var err error

	switch state {
	case Started:
		err = sqlite3x.ReplicationBegin(t.conn)
	case Writing:
		frames := args[0].(*sqlite3x.ReplicationWalFramesParams)
		err = sqlite3x.ReplicationWalFrames(t.conn, frames)
	case Undoing:
		err = sqlite3x.ReplicationUndo(t.conn)
	case Ended:
		err = sqlite3x.ReplicationEnd(t.conn)
	case Stale:
	}

	return err
}

// Assert that we have actually entered a cricial section via mutex
// locking.
func (t *Txn) checkEntered() {
	if !t.entered {
		panic(fmt.Sprintf("accessing or modifying txn state without mutex: %s", t.id))
	}
}

type txnState struct {
	state fsm.State
}

// CurrentState returns the current state, implementing fsm.Stater.
func (s *txnState) CurrentState() fsm.State {
	return s.state
}

// SetState switches the current state, implementing fsm.Stater.
func (s *txnState) SetState(state fsm.State) {
	s.state = state
}

// Capture valid state transitions within a transaction.
func newTxnStateRules(forLeader bool) *fsm.Ruleset {
	rules := &fsm.Ruleset{}

	// Add all rules common to both leader and follower transactions.
	for o, states := range transitions {
		for _, e := range states {
			rules.AddTransition(fsm.T{O: o, E: e})
		}
	}

	// Add rules valid only for leader transactions.
	if forLeader {
		for _, state := range []fsm.State{Pending, Ended} {
			rules.AddTransition(fsm.T{
				O: state,
				E: Stale,
			})
		}
	}

	return rules
}

// Map of all valid state transitions.
var transitions = map[fsm.State][]fsm.State{
	Pending: {Started},
	Started: {Writing, Undoing, Ended},
	Writing: {Writing, Undoing, Ended},
	Undoing: {Undoing, Ended},
}