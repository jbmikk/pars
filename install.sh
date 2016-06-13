#Create vendor directory
mkdir -p vendor
cd vendor

if [ ! -d "dbg" ]; then
#Install dbg.h
git clone git@github.com:jbmikk/dbg.git --depth 1
fi
