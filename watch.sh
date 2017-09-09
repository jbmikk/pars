#!/bin/bash

while inotifywait -e close_write ./pars ./cli_test; do sh test.sh; done
