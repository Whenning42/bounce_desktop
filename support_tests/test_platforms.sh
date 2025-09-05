echo "Test X11"
mkdir -p out/on_x11
export PLATFORM_INFO_OUTDIR=out/on_x11
startx ./run_test.sh

echo "Test Weston"
mkdir -p out/on_weston
export PLATFORM_INFO_OUTDIR=out/on_weston
weston -- ./run_test.sh

echo "Test Console"
mkdir -p out/on_console
export PLATFORM_INFO_OUTDIR=out/on_console
./run_test.sh
