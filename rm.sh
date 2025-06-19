ps ajx | grep serverd | grep -v grep | awk '{print $1}' | xargs sudo kill -9
cd cgi_wp/message/
make clean
cd ../../
make clean