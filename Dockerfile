# Same image TWiLight Menu++ pins for its builds.
FROM devkitpro/devkitarm:20241104

# Debian bullseye left LTS, so resolve packages from the snapshot the image was built against.
RUN sed -i -e 's|^deb http://deb.debian.org|# &|' -e 's|^# deb http://snapshot.debian.org|deb http://snapshot.debian.org|' \
        /etc/apt/sources.list \
 && apt-get -o Acquire::Check-Valid-Until=false update \
 && apt-get install -y --no-install-recommends g++ \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /project
