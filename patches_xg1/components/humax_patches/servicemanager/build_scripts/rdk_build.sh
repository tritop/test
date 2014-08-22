#!/bin/bash

#######################################
#
# Build Framework standard script for
#
# SDK component

# use -e to fail on any shell issue
# -e is the requirement from Build Framework
set -e


# default PATHs - use `man readlink` for more info
# the path to combined build
export RDK_PROJECT_ROOT_PATH=${RDK_PROJECT_ROOT_PATH-`readlink -m ../..`}
export COMBINED_ROOT=$RDK_PROJECT_ROOT_PATH

# path to build script (this script)
export RDK_SCRIPTS_PATH=${RDK_SCRIPTS_PATH-`readlink -m $0 | xargs dirname`}

# path to components sources and target
export RDK_SOURCE_PATH=${RDK_SOURCE_PATH-`readlink -m .`}
export RDK_TARGET_PATH=${RDK_TARGET_PATH-$RDK_SOURCE_PATH}

# fsroot and toolchain (valid for all devices)
export RDK_FSROOT_PATH=${RDK_FSROOT_PATH-`readlink -m $RDK_PROJECT_ROOT_PATH/sdk/fsroot/ramdisk`}
export RDK_TOOLCHAIN_PATH=${RDK_TOOLCHAIN_PATH-`readlink -m $RDK_PROJECT_ROOT_PATH/sdk/toolchain/staging_dir`}


# default component name
export RDK_COMPONENT_NAME=${RDK_COMPONENT_NAME-`basename $RDK_SOURCE_PATH`}


# parse arguments
INITIAL_ARGS=$@

CC_DEVICE=$RDK_PLATFORM_DEVICE

if [ -z "$CC_DEVICE" ]; then
    echo "--platform-device option is required to build this component"
    exit 1
fi

DEBUGAPP=0
UPLOAD=0
REBUILD=0
JOBS_NUM=0 # 0 means detect automatically
PROJECT_CONFIG=()

function usage()
{
    set +x
    echo "Usage: `basename $0` [-h|--help] [-v|--verbose] [-d|--debug] [action]"
    echo "    -h    --help                  : this help"
    echo "    -v    --verbose               : verbose output"
    echo "    -d    --debug                 : debug"
    echo "    -u    --upload-symbols        : upload symbols"
    echo "    -j                            : specify number of jobs to be used by make"
    echo
    echo "Supported actions:"
    echo "      configure, clean, build (DEFAULT), rebuild, install"
}

# options may be followed by one colon to indicate they have a required argument
if ! GETOPT=$(getopt -n "build.sh" -o hvduj: -l help,verbose,debug,upload-symbols -- "$@")
then
    usage
    exit 1
fi

eval set -- "$GETOPT"

while true; do
  case "$1" in
    -h | --help ) usage; exit 0 ;;
    -v | --verbose ) set -x ;;
    --platform-device ) CC_DEVICE="$2" ; shift ;;
    -d | --debug ) DEBUGAPP=1 ;;
    -u | --upload-symbols ) UPLOAD=1 ;;
    -j ) JOBS_NUM=$OPTARG ;;

    -- ) shift; break;;
    * ) break;;
  esac
  shift
done

ARGS=$@


# component-specific vars
CC_PATH=$RDK_SOURCE_PATH
export SRVCMNGR_BUILD_DIR=$RDK_PROJECT_ROOT_PATH/servicemanager/build/servicemanager
export FSROOT=${RDK_FSROOT_PATH}
export TOOLCHAIN_DIR=${RDK_TOOLCHAIN_PATH}
export WORK_DIR=$RDK_PROJECT_ROOT_PATH/work${CC_DEVICE^^}

export QT_SRC_ROOT=${RDK_PROJECT_ROOT_PATH}/opensource/qt
source $QT_SRC_ROOT/apps_helpers.sh

# functional modules

function configure()
{
    true #use this function to perform any pre-build configuration
}

function clean()
{
    CURR_DIR=`pwd`
    cd ${SRVCMNGR_BUILD_DIR}

    source ${RDK_PROJECT_ROOT_PATH}/build_scripts/setBCMenv.sh

    source ${QT_SRC_ROOT}/setenv.sh

    set_qmakespec

    [[ -e Makefile ]] && make distclean

    cd $CURR_DIR
}

function build()
{
    CURR_DIR=`pwd`
    cd ${SRVCMNGR_BUILD_DIR}

    export ParkerSI_ROOT=$BUILDS_DIR

    # apps_helpers.sh uses DEBUG variable, it'll be overridden before
    # sourcing setBCMenv.sh
    if [[ "$DEBUGAPP" -eq "1" ]] ; then
      export DEBUG=1
    fi

    if [ -d "$BUILDS_DIR/$COMBINED_DIR/sdk/fsroot/ramdisk" ];
    then
       export FSROOT=$BUILDS_DIR/$COMBINED_DIR/sdk/fsroot/ramdisk
    fi

    # handle default flags
    handleScriptOptions

    export DEBUG=n
    if [[ "$DEBUGAPP" -eq "1" ]] ; then
      export DEBUG=y
    fi

    source ${RDK_PROJECT_ROOT_PATH}/build_scripts/setBCMenv.sh

    # setup enviroment for build
    source $QT_SRC_ROOT/setenv.sh
    source ${RDK_PROJECT_ROOT_PATH}/build_scripts/buildQtAppsHelpers.sh

    echo "Building Service Manager..."

    if [[ $REBUILD -eq "1" ]]; then
        echo "Requested clean build...."
        set_qmakespec
        [[ -e Makefile ]] && make distclean
    fi

    configureProject servicemanager.pro ${PROJECT_CONFIG[@]}

    # fix-me: get rid of makefile modification
    sed -e 's/-lGL//g' <Makefile >Makefile_temp
    sed '/INCPATH       =/a\
            include ${NEXUS_BIN_DIR}/include/platform_app.inc\
            CXXFLAGS += $(filter-out -Wstrict-prototypes -std=c89 -pedantic,$(NEXUS_CFLAGS)) $(addprefix -D,$(NEXUS_APP_DEFINES))\
            CFLAGS += $(NEXUS_CFLAGS) $(addprefix -D,$(NEXUS_APP_DEFINES))\
            INCPATH += $(addprefix -I,$(NEXUS_APP_INCLUDE_PATHS))' <Makefile_temp >Makefile

    rm -f Makefile_temp
    # end of fix-me

    export VOB_TOP=$WORK_DIR/Refsw

    make -w -j$JOBS_NUM all

    cd $CURR_DIR
}

function rebuild()
{
    REBUILD=1
    build
}

function install()
{
    INSTALL_PATH=${RDK_FSROOT_PATH}usr/local/lib

    cd ${RDK_SCRIPTS_PATH}../

    cp -Rfl build/servicemanager/*.so* ${INSTALL_PATH}
}


# run the logic

#these args are what left untouched after parse_args
HIT=false

for i in "$ARGS"; do
    case $i in
        configure)  HIT=true; configure ;;
        clean)      HIT=true; clean ;;
        build)      HIT=true; build ;;
        rebuild)    HIT=true; rebuild ;;
        install)    HIT=true; install ;;
        *)
            #skip unknown
        ;;
    esac
done

# if not HIT do build by default
if ! $HIT; then
  build
fi
