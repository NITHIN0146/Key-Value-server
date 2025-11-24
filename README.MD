# Steps to run #
- sudo systemctl start postgresql
- pgrep -f postgres | xargs -I {} sudo taskset -cp 0 {}
- taskset -c 1 ./server
- taskset -c 7-9 ./run.sh load.js
- htop
