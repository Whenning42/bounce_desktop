FILE=eglinfo.txt
echo "Diffs for file: ${FILE}"
sdiff -s out/on_console/${FILE} out/on_weston/${FILE}
sdiff -s out/on_console/${FILE} out/on_x11/${FILE}

FILE=glxinfo.txt
echo "Diffs for file: ${FILE}"
sdiff -s out/on_console/${FILE} out/on_weston/${FILE}
sdiff -s out/on_console/${FILE} out/on_x11/${FILE}

FILE=vkinfo.txt
echo "Diffs for file: ${FILE}"
sdiff -s out/on_console/${FILE} out/on_weston/${FILE}
sdiff -s out/on_console/${FILE} out/on_x11/${FILE}
