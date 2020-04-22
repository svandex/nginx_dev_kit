#!/bin/sh

PWD=$(pwd)

SRCREPO="/mnt/c/Users/svandex/Repositories"

NGX="/home/svandex/nginx_src"

ModulePath="${SRCREPO}/nginx_dev_kit/module"

NGXCPP="${SRCREPO}/ngx_cpp_dev/ngxpp"

PREFIX="/home/svandex/build/nginx"

# enter into nginx source directory to configure
cd ${NGX}

./configure --add-module=${NGXCPP} --add-dynamic-module="${ModulePath}/ngx_module_basic" --prefix=${PREFIX} --with-compat

make -f objs/Makefile install -B

cd ${PWD}
