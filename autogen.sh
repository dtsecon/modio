#! /bin/sh
if autoreconf --verbose --install --force; then
	echo
	echo "------------------------------------------------------"
	echo "Initialized build system. You can now run ./configure "
	echo "------------------------------------------------------"
	echo
else
	echo
	echo "--------------------------"
	echo "Running autoreconf failed."
	echo "--------------------------"
	echo
fi
