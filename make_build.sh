mkdir -p ./build
VERSION=`git describe --tags --long`
DATE=`date +%Y-%m-%d`

if [ -z $VESRION ]; then
    VERSION="local"
fi
arduino-cli compile --fqbn esp32:esp32:esp32c6 --build-property "build.extra_flags=\"-DSTRING_VERSION=\"$VERSION\"\" -DSTRING_DATE=\"$DATE\"" --output-dir ./build/ HollowClock5Plus.ino -v
