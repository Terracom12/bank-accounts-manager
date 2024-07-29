#!/bin/bash

tidy_files=$(find src -type f -name '*.cpp')
format_files="$(find include -type f -name '*.h*') $tidy_files"

{
	clang-tidy -header-filter=$(pwd)/include $tidy_files --warnings-as-errors=* && \
		tidy_result=0
} || {
	tidy_result=1
}

{
	clang-format $format_files -Werror --dry-run && \
		format_result=0
} || {
	format_result=1
}

exit $((tidy_result | format_result))
