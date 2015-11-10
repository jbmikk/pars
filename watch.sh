while inotifywait -e close_write ./stlib ./pars ./teststlib ./testpars; do sh test.sh; done
