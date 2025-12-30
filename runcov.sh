#!/bin/sh

cov-build --dir cov-int make
tar czvf swirc.tgz cov-int
