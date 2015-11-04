#!/usr/bin/env bash

# run the server, save its PID
eval "./homework4 8001 WWW &"
pid=$!

# now fetch the index page from the server
eval "wget localhost:8001/index.html -O myserver.html"

# now that we're done, kill the server
kill -KILL $pid
