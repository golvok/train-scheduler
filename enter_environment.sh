
if [ ! -d "src" ] || [ ! -f "src/Makefile" ] || [ ! -d "test" ]; then
	echo "please execute in the top level directory; where src/ and test/ are"
	exit 1;
fi

if [ -z "$SHELL" ]; then
	echo "WARNING: \$SHELL not set, using bash"
	SHELL_EXE="bash";
else
	SHELL_EXE="$SHELL";
fi

if [ ! -z "$TRAIN_SCH_ROOTDIR" ]; then
	echo "WARNING: invocation from already initialized environment. Old TRAIN_SCH_ROOTDIR=$TRAIN_SCH_ROOTDIR";
fi

export TRAIN_SCH_ROOTDIR="$(readlink --canonicalize "$PWD")";
export TRAIN_SCH_EXEDIR="$TRAIN_SCH_ROOTDIR/build/EXE";

export PATH="$PATH:$TRAIN_SCH_EXEDIR"

echo "Entering $SHELL_EXE with setup environment. TRAIN_SCH_ROOTDIR=$TRAIN_SCH_ROOTDIR";
$SHELL_EXE
echo "Exiting environment. Old TRAIN_SCH_ROOTDIR=$TRAIN_SCH_ROOTDIR";
