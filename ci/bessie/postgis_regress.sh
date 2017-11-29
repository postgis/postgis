#BRANCH is passed in via jenkins which is set via svn hook
export PATH=${PATH}:/usr/local:/usr/local/lib:/usr/local/bin
sh autogen.sh
./configure --with-projdir=/usr/local --with-libiconv=/usr/local --without-interrupt-tests
make clean
make
export PGUSER=postgres
export PGIS_REG_TMPDIR=~/tmp/pgis_reg_${BRANCH}
make check
sudo make install
make check RUNTESTFLAGS=--extension
