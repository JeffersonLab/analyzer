FROM centos:centos7

ARG APP_VERSION
ARG REPO_NAME

RUN yum -y install git && \
    yum -y groupinstall 'Development Tools'&& \
    yum -y install gcc-c++ && \
    yum -y install make \
    yum install -y root \
    && localedef -i en_US -f UTF-8 en_US.UTF-8

ADD https://github.com/Kitware/CMake/releases/download/v3.22.2/cmake-3.22.2-linux-x86_64.tar.gz .
RUN tar -xvf cmake-3.22.2-linux-x86_64.tar.gz
RUN mv cmake-3.22.2-linux-x86_64 /usr/local/cmake
RUN ls -l /usr/local/cmake/bin
ENV PATH="/usr/local/cmake/bin:$PATH"
ADD https://github.com/panta-123/${REPO_NAME}/archive/refs/tags/${APP_VERSION}.tar.gz .
RUN tar -xvf ${APP_VERSION}.tar.gz
WORKDIR "/${REPO_NAME}-${APP_VERSION}"
RUN mkdir -p $HOME/local/analyzer
RUN cmake -DCMAKE_INSTALL_PREFIX=$HOME/local/analyzer -B builddir -S /${REPO_NAME}-${APP_VERSION}/
RUN cmake --build builddir -j8
RUN cmake --install builddir
ENV PATH="~/local/analyzer/bin:$PATH"
ENV LD_LIBRARY_PATH="~/local/analyzer/lib:$LD_LIBRARY_PATH"

