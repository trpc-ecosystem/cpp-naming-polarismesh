# Polarismesh etcd provider demo

This example demonstrates how to use etcd to store and retrieve key-value pairs in a client and server program with Polarismesh. The example includes a simple client and server program.

## File Structure

```shell
$ tree examples/config/etcd/
examples/config/etcd/
├── client
│   ├── BUILD
│   ├── client.cc
│   └── trpc_cpp_fiber.yaml
└── server
    ├── BUILD
    ├── greeter_service.cc
    ├── greeter_service.h
    ├── helloworld.proto
    ├── helloworld_server.cc
    └── trpc_cpp_fiber.yaml
└── run.sh
```
## Running the Example

* Prerequisites

  You need to set up a single-node Polarismesh service discovery cluster. Please follow the instructions in the [Polarismesh Single-node Installation Guide](https://polarismesh.cn/docs/) to install and configure the cluster. The installation process includes the following steps:

  - Install Docker and Docker Compose
  - Download the Polarismesh Docker Compose file
  - Configure the Polarismesh cluster
  - Start the Polarismesh cluster using Docker Compose
  - Verify that the Polarismesh cluster is running
  - Once you have completed the installation and configuration, you can proceed with the example.

* Compilation

We can run the following command to compile the demo.

```shell
bazel build //examples/...
```

*  Run the server program.

We can run the following command to start the server program.

```shell
./bazel-bin/examples/server/helloworld_server --config=examples/server/trpc_cpp_fiber.yaml
```

* Run the client program

We can run the following command to start the client program.

```shell
./bazel-bin/examples/client/client --config=examples/client/trpc_cpp_fiber.yaml
```

* View the polarismesh data

If you have deployed a single-node Polarismesh service discovery system following the reference documentation mentioned above, you can log in to the console at 127.0.0.1:8080 to view the backend service status of our example deployment.
