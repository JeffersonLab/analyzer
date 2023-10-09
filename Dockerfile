FROM centos:centos7

ARG APP_VERSION
ARG REPO_NAME

RUN yum update -q -y

RUN yum -y install epel-release &&\
    yum -y install git && \
    yum -y groupinstall 'Development Tools'&& \
    yum -y install gcc-c++ && \
    yum -y install make && \
    yum install -y root && \
    yum install -y which && \
    yum install -y root-montecarlo-eg &&  \
    localedef -i en_US -f UTF-8 en_US.UTF-8

ADD https://github.com/Kitware/CMake/releases/download/v3.22.2/cmake-3.22.2-linux-x86_64.tar.gz .
RUN tar -xvf cmake-3.22.2-linux-x86_64.tar.gz && rm cmake-3.22.2-linux-x86_64.tar.gz
RUN mv cmake-3.22.2-linux-x86_64 /usr/local/cmake
ENV PATH="/usr/local/cmake/bin:$PATH"
ADD https://github.com/JeffersonLab/${REPO_NAME}/archive/refs/tags/${APP_VERSION}.tar.gz .
RUN tar -xvf ${APP_VERSION}.tar.gz && rm ${APP_VERSION}.tar.gz
WORKDIR "/${REPO_NAME}-${APP_VERSION}"
RUN cmake -DCMAKE_INSTALL_PREFIX=/usr/local/analyzer -B BUILD -S /${REPO_NAME}-${APP_VERSION}/
RUN cmake --build BUILD -j8
RUN cmake --install BUILD
ENV LD_LIBRARY_PATH="/usr/local/analyzer/lib64:$LD_LIBRARY_PATH"
ENV ANALYZER="/usr/local/analyzer"
ENV PATH="/usr/local/analyzer/bin:/usr/bin/root:$PATH"
ENV ROOTSYS="/usr/"

