export UID=$(id -u)
export GID=$(id -g)
docker-compose -f $(dirname "$0")/docker-compose.arch.yml build
docker-compose -f $(dirname "$0")/docker-compose.arch.yml run z-mx zsh 
