

if [ -z "$PYTHONPATH" ]
then
   PYTHONPATH=""
fi

# Setup python path
export PYTHONPATH=${PWD}/python:${PYTHONPATH}


