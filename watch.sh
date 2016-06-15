while inotifywait -e close_write ./pars ./testpars; do sh test.sh; done
