FROM pdockerdevenv as z-mx-resources

USER root
# download clang
RUN wget https://github.com/llvm/llvm-project/releases/download/llvmorg-12.0.0/clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-20.04.tar.xz # do not untar it in this image stage
RUN git clone https://github.com/google/flatbuffers.git
RUN git clone https://github.com/tplgy/cppcodec
# RUN git clone --depth 1 --branch binutils-2_36_1 git://sourceware.org/git/binutils-gdb.git 
RUN wget http://ftp.gnu.org/gnu/binutils/binutils-2.36.tar.xz

# ----------------------------------------
FROM pdockerdevenv as main

USER root

RUN apt-get update && apt-get install -y cmake gdb git mercurial libhwloc-dev libck-dev openjdk-8-jdk maven bear zlib1g-dev libssl-dev autoconf libpcre3-dev libreadline-dev libpcap-dev libmbedtls-dev liblz4-dev curl xz-utils build-essential

COPY --from=z-mx-resources /clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-20.04.tar.xz /
RUN tar -xf /clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-20.04.tar.xz && mv clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-20.04 /usr/local/llvm_12.0.0

# build and install binutils 2.36.1
COPY --from=z-mx-resources /binutils-2.36.tar.xz /binutils-2.36.tar.xz 
RUN tar -xf /binutils-2.36.tar.xz
RUN cd /binutils-2.36 && ./configure --with-pic && make -j8
RUN cd /binutils-2.36 && make install

RUN apt-get install -y sudo
ARG UNAME=testuser
ARG UID
ARG GID
RUN passwd -d $UNAME
RUN adduser $UNAME sudo
USER $UNAME

# ENV CXX=clang++
# ENV CC=clang
# ENV PATH=/usr/local/llvm_12.0.0/bin:${PATH}
# ENV LD_LIBRARY_PATH=/usr/local/llvm_12.0.0/lib:${LD_LIBRARY_PATH}

RUN mkdir -p /home/$UNAME/Documents/github
COPY --from=z-mx-resources --chown=$UID:$GID /flatbuffers /home/$UNAME/Documents/github/flatbuffers
COPY --from=z-mx-resources --chown=$UID:$GID /cppcodec /home/$UNAME/Documents/github/cppcodec
WORKDIR /home/$UNAME/Documents
RUN cd /home/$UNAME/Documents/github/flatbuffers && cmake .
RUN cd /home/$UNAME/Documents/github/flatbuffers && make -j8
RUN cd /home/$UNAME/Documents/github/cppcodec && git submodule update --init && cmake .

USER root
RUN cd /home/$UNAME/Documents/github/flatbuffers && make install
RUN cd /home/$UNAME/Documents/github/cppcodec && make install
USER $UNAME
