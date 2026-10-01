README DOCKER

```bash
sudo docker build -t docker_runner .
sudo docker run -it docker_runner /bin/bash
sudo docker rm -f docker_runner && docker rmi docker_runner

```

```bash
# Inside Docker
gitlab-runner start
gitlab-runner register

# Fill with server informations and token
# Executor: shell
gitlab-runner --debug run
```


