while inotifywait -e close_write ./clib ./pars ./testclib ./testpars; do sh test.sh; done
