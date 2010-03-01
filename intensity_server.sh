# Do a chdir to the dir with this script in it
abspath="$(cd "${0%/*}" 2>/dev/null; echo "$PWD"/"${0##*/}")"
path_only=`dirname "$abspath"`
cd ${path_only}
exec ./cbuild/src/server/Intensity_CServer $@

