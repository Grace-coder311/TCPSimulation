# create "visible symbols" (functions only) files to be used
# when making header files.
echo "Creating visible symbol files for headers."
ctags -x --c-kinds=f --file-scope=no *.c | awk -f headers.awk
date > headers
echo "Done with visible symbols."
