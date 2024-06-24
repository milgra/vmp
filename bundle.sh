rm -rf vmp.app
mkdir vmp.app
mkdir -p vmp.app/bin
mkdir -p vmp.app/lib
mkdir -p vmp.app/res
ldd build/vmp | awk '{print $3}' | xargs -I {} cp {} vmp.app/lib/
rm vmp.app/lib/libc.so.*
rm vmp.app/lib/libm.so.*
cp -R build/vmp vmp.app/bin/
cp -R res/html vmp.app/res/
cp -R res/img vmp.app/res/
cp res/vmp.desktop vmp.app/
cp res/vmp.png vmp.app/
echo 'PTH=$(dirname $(readlink -f $0))' > vmp.app/vmp
echo "LD_LIBRARY_PATH=\$PTH/lib \$PTH/bin/vmp -r \$PTH/res" >> vmp.app/vmp
chmod +x vmp.app/vmp
tar czf vmp.app.tar.gz vmp.app
