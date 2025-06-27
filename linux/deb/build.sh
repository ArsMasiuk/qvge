mkdir qvge_0.7.0/usr/local/bin

cp ../../src/bin/qvge qvge_0.7.0/usr/local/bin

chmod +x qvge_0.7.0/usr/local/bin/qvge

dpkg-deb --build qvge_0.7.0
