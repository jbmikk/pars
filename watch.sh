#!/bin/bash

while inotifywait -e close_write ./pars ./test_cli; do sh test.sh; done
