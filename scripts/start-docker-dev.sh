#!/bin/bash

# =======================================================================
#          author: alexan                                               #
#          date: 18/5/2024                                              #
#          function: boot docker containter and config info             #
# =======================================================================

ROOT_PATH=$(cd `dirname $0`; pwd)

# System avaliable cpu set
S_CPU=$(lscpu | awk '/On-line CPU\(s\) list:/ {print $4}' | cut -d'-' -f1)
E_CPU=$(lscpu | awk '/On-line CPU\(s\) list:/ {print $4}' | cut -d'-' -f2)

# Config parameters
USE_CPUS=
CONTAINTER_WORKSPACE="/opt/platform"
DOCKER_IMAGE=
CONTAINTER_NAME="caa_platform"
PROJECT_PATH=$ROOT_PATH/

# recontruct echo_message function
function echo_message()
{
    # Define colors
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[0;33m'
    BLUE='\033[0;34m'
    RESET='\033[0m' # No Color

    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -r|--red)
                shift
                echo -e "${RED}$1${RESET}"
                ;;
            -g|--green)
                shift
                echo -e "${GREEN}$1${RESET}"
                ;;
            -y|--yellow)
                shift
                echo -e "${YELLOW}$1${RESET}"
                ;;
            -b|--blue)
                shift
                echo -e "${BLUE}$1${RESET}"
                ;;
            *)
                echo "$1"
                ;;
        esac
        shift
    done
}

function help_menu()
{
	SCRIPT_NAME=$(basename "$0")
	echo_message "usage: ${SCRIPT_NAME} [-chpwli] [--cpu_set] [--image_name] [--docker_name]"
	echo_message "       			       [--project_path] [--work_path] [--help]"
	echo_message "       			       [--list_cpu] [--info]"
	echo_message ""
	echo_message "       ${SCRIPT_NAME} [--cpu_set=x-y --image_name=i_name --docker_name=d_env_name]"
	echo_message "       ${SCRIPT_NAME} [--project_path=absolute_path --work_path=absolute_path]"
	echo_message ""
	echo_message -e "		           -l, list_cpu			list os own cpu set"
	echo_message -e "		           -i, info			list use based info"
	
	echo_message "example config cpu_set:"
	echo_message "	./${SCRIPT_NAME} -c=2-4"
	exit -1
}

function config_cpu_set()
{
	cpu_set=$1
	
	# check nullptr
	if [ -z "${cpu_set}" ]; then
		echo_message -r "cpu_set is null"
		exit -1
	fi

	# chekct cpu set format
	if [[ ! ${cpu_set} =~ ^[0-9]+-[0-9]+$ ]]; then
		echo_message -r "invalid cpu_set format"
		exit -1
	fi
  
	s_cpu=$(echo_message ${cpu_set} | cut -d'-' -f1)
	e_cpu=$(echo_message ${cpu_set} | cut -d'-' -f2)
	
	# check border
	if [ "$s_cpu" \< "$S_CPU" ] || [ "$e_cpu" \> "$E_CPU" ]; then
		echo_message -r "input cpu_set {${s_cpu}, ${e_cpu}} not in system cpu set {${S_CPU}, ${E_CPU}}"
		exit -1
	fi
	
	# assign global variable
	USE_CPU="${s_cpu}-${e_cpu}"
}

function config_image_name()
{
	DOCKER_IMAGE=$1
	image_name=${DOCKER_IMAGE%%:*}
	image_tag=${DOCKER_IMAGE#*:}
	
	# check nullptr
	if [ -z "${DOCKER_IMAGE}" ]; then
		echo_message -r "image name is null"
		exit -1
	fi
	
	# check exists
	e_image=$(docker images | grep "${image_name}")
	if [[ -z ${e_image} ]]; then
		echo_message -r "image not exists"
		exit -1
	fi
	
	e_image_tag=$(docker images | grep "${image_name}" | awk '{print $2}')
	if [[ ! "${e_image_tag}" = "${image_tag#*:}" ]]; then
		echo_message -r "image version is error"
		exit -1
	fi
}

function config_containter_name()
{
	CONTAINTER_NAME=$1
	
	# check nullptr
	if [ -z "${CONTAINTER_NAME}" ]; then
		echo_message -r "docker containter name is null"
		exit -1
	fi
}

function config_project_path()
{
	PROJECT_PATH=$1

	# check nullptr
	if [ -z "${PROJECT_PATH}" ]; then
		echo_message -r "project path is null"
		exit -1
	fi
	
	# check absolute path
	if [[ ! "${PROJECT_PATH}" =~ ^/.* ]]; then
		echo_message -r "project path requires absolute path"
		exit -1
	fi
	
	# check exists
	if [ ! -d "${PROJECT_PATH}" ]; then
		echo_message -r "project path requires exist"
		exit -1
	fi
}

function config_work_path()
{
	CONTAINTER_WORKSPACE=$1

	# check nullptr
	if [ -z "${CONTAINTER_WORKSPACE}" ]; then
		echo_message -r "project path is null"
		exit -1
	fi
	
	# check absolute path
	if [[ ! "${CONTAINTER_WORKSPACE}" =~ ^/.* ]]; then
		echo_message -r "work path requires absolute path"
		exit -1
	fi
}

function list_cpu()
{
	echo_message -y "system cpu_set is {${S_CPU}, ${E_CPU}}"
	exit -1
}

function print_test()
{
	echo_message -g "=========================================="
	echo_message -g "ROOT_PATH=${ROOT_PATH}"
	echo_message -g "system cpu_set={${S_CPU}, ${E_CPU}}"
	echo_message -g "USE_CPU=${USE_CPU}"
	echo_message -g "CONTAINTER_WORKSPACE=${CONTAINTER_WORKSPACE}"
	echo_message -g "DOCKER_IMAGE=${DOCKER_IMAGE}"
	echo_message -g "CONTAINTER_NAME=${CONTAINTER_NAME}"
	echo_message -g "PROJECT_PATH=${PROJECT_PATH}"
	echo_message -g "=========================================="
	exit -1
}

function start_containter()
{
	systemctl daemon-reload
	
	if [ -z "${DOCKER_IMAGE}" ] || [ -z "${USE_CPU}" ]; then
		echo_message -r "boot docker lack of image name or cpu_set"
		exit -1
	fi

	while true; do
		DOCKER_RUNNING=`docker ps -f name=$CONTAINTER_NAME | awk 'NR==2{print \$NF}'`
		if [ "$DOCKER_RUNNING" != "$CONTAINTER_NAME" ]; then
			docker rm -f $CONTAINTER_NAME 2>1 >/dev/null
			docker run -tid --privileged --cpuset-cpus="$USE_CPUS" --net=host \
				-w "$CONTAINTER_WORKSPACE" \
				-v "$PROJECT_PATH:$CONTAINTER_WORKSPACE" \
				-v /sys/bus/pci:/sys/bus/pci \
				--name $CONTAINTER_NAME $DOCKER_IMAGE bash
		fi
		break
	done
}

function main()
{

	# 使用getopt解析参数
	ARGS=$(getopt -o h::c::,p::,w::,l::,i:: -l "help::, cpu_set::, project_path::, work_path::, image_name::, docker_name::, --list_cpu, --info::" --name "$0" -- "$@")

	# 处理参数解析后的结果
	eval set -- "$ARGS"

	# 默认值
	A_VALUE=
	B_VALUE=

	# 解析参数
	while true; do
		case "$1" in
			-h | --help)
				help_menu
				shift 2
				;;
			-c | --cpu_set)
				case $2 in
				'')
					echo_message "please input non nullptr cpu_set"
					exit -1
					;;
				*)
					config_cpu_set ${2#*=}
					shift 2
					;;
				esac
				;;
			-p | --project_path)
				case $2 in
				'')
					echo_message "please input project path"
					exit -1
					;;
				*)
					config_project_path ${2#*=}
					shift 2
					;;
				esac
				;;
			-w | --work_path)
				case $2 in
				'')
					echo_message "please input project path"
					exit -1
					;;
				*)
					config_work_path ${2#*=}
					shift 2
					;;
				esac
				;;
			--image_name)
				case $2 in
				'')
					echo_message "please input docker image name"
					exit -1
					;;
				*)
					config_image_name ${2#*=}
					shift 2
					;;
				esac
				;;
			--docker_name)
				case $2 in
				'')
					echo_message "please input docker containter name"
					exit -1
					;;
				*)
					config_containter_name ${2#*=}
					shift 2
					;;
				esac
				;;
			-l | --list_cpu)
				list_cpu
				shift 2
				;;
			-i | --info)
				print_test
				shift 2
				;;
			--)
				shift
				break
				;;
			*)
				echo_message "Internal error!"
				exit 1
				;;
		esac
	done
}

main $@
start_containter
