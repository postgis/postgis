FROM postgis_build_env

ADD . /postgis
USER root
RUN chown --recursive postgres /postgis
USER postgres

ENV RUNTESTFLAGS=-v
CMD /usr/local/pgsql/bin/pg_ctl start && \
    cd postgis && \
    ./autogen.sh && \ 
    ./configure && \ 
    make && \
    make check && \
    make check RUNTESTFLAGS='--dumprestore' && \
    sudo make install && \
    make installcheck && \
    make installcheck RUNTESTFLAGS='--dumprestore'

