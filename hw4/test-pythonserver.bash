#!/usr/bin/env bash

# save current directory and go to WWW
pushd .
cd WWW

# run the server, save its PID
eval "python -m SimpleHTTPServer 8002 &"
pid=$!


# wait a bit
sleep 2

# go back to original directory
popd
# now fetch the index page from the server
eval "wget localhost:8002/index.html -O pythonserver.html"

# now that we're done, kill the server
kill -KILL $pid
