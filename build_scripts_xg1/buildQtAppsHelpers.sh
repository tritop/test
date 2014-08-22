#!/bin/bash
[[ ! -n "$COMBINED_ROOT" ]] && export COMBINED_ROOT=$(cd .. && pwd)
[[ ! -n "$WORK_DIR" ]] && export WORK_DIR=$(cd ../workXG1 && pwd)


function findQt()
{
    #QT_PREFIX=$QT_SRC_ROOT/stage/usr/local/Qt
     QT_PREFIX=$WORK_DIR/Refsw/AppLibs/opensource/qt/qt-everywhere-opensource-src-5.1.0-host
     #QT_PREFIX=${QT_GENERIC}-${QT_VERSION}-host

    if [ -e $QT_PREFIX/bin/qmake ]; then
        export QT_PREFIX
        return
    fi
    QT_PREFIX=$FSROOT/usr/local/Qt
    if [ -e $QT_PREFIX/bin/qmake ]; then
        export QT_PREFIX
        return
    fi
    unset QT_PREFIX
    echo "Cannot find qt"
    exit -1
}

function set_qmakespec()
{
    # find qt installatin and export QT_PREFIX
    findQt
    export QMAKESPEC=$QT_PREFIX/mkspecs/devices/linux-broadcom-v3d-g++    
}
function configureProject()
{
    local project_file=${1}; shift
    local project_config=${@}
    set_qmakespec
    $QT_PREFIX/bin/qmake ${project_file} ${project_config[@]}
    unset QT_PREFIX
}

function nextFd()
{
    local fd=3
    local fd_MAX=$(ulimit -n)
    while [[ $fd -le $fd_MAX && -e /proc/$$/fd/$fd ]]; do
        ((++fd))
    done
    if [ $fd -gt $fd_MAX ]; then
        echo "Could not find available file descriptor" >&2
        echo "-1"
    else
        echo "$fd"
    fi
}

function redirectStdOutAndStdErr()
{
    local logFile=$1

    [[ -n "$logFile" ]] || { echo "Cannot redirect std{out/err}"; return; }

    echo "Please see the status of build at $logFile"

    # clear log file
    cat /dev/null > $logFile

    # save trace fd if it points to stdout or stderr
    if [[ $TRACEFD -le 2 ]]; then
        local fd=$(nextFd)
        if ! eval "exec $fd>&$TRACEFD"; then
            exit -1
        fi
        export TRACEFD=$fd
        export BASH_XTRACEFD=$TRACEFD
    fi

    # redirect std output
    exec 1>>$logFile 2>>$logFile
}

function handleScriptOptions()
{
    if [[ "$JOBS_NUM" -le "0" ]] ; then
        JOBS_NUM=$((`grep -c ^processor /proc/cpuinfo` + 1))
    fi
    # TODO: uploading and dumping of debug symbols for crash portal
    #       should be separated
    if [[ "$UPLOAD" -eq "1" ]] ; then
        # check env needed for extractSymbols.sh
        [[ ! -n "$STRIP" ]] && export STRIP=mipsel-linux-strip
        [[ ! -n "$DUMP_SYMS" ]] && export DUMP_SYMS=$SCRIPTS_DIR/tools/linux/dump_syms
        export UPLOAD_FLAG=1
        export QT_BREAKPAD_ROOT_PATH=$SCRIPTS_DIR
    fi
    if [[ "$DEBUGAPP" -eq "1" ]] ; then
        PROJECT_CONFIG+=("CONFIG+=debug")
    else
        PROJECT_CONFIG+=("CONFIG+=silent")
    fi
}

function trap_exit()
{
    exit_code=$?
    if [ $exit_code -ne 0 ]; then
        echo "Build failed" >&$TRACEFD
    fi
    trap - INT TERM EXIT
    exit $exit_code
}
trap 'trap_exit' INT TERM EXIT
set -e
# set -x

# this to be used to print messages for console output on jenkins
export TRACEFD=2

# set fd for output generated when set -x is enabled
export BASH_XTRACEFD=$TRACEFD

source $WORK_DIR/../build_scripts/setBCMenv.sh
