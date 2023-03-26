FROM henne90gen/gtk:4.8

RUN apt-get update && apt-get dist-upgrade -y && apt-get install -y poppler-utils

WORKDIR /app/build

# NOTE Hack for git inside docker (see https://stackoverflow.com/questions/72978485/git-submodule-update-failed-with-fatal-detected-dubious-ownership-in-repositor)
RUN git config --global --add safe.directory '*'
