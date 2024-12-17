# Prometheus plugin

Expose Halon process statistics as [Prometheus](https://prometheus.io) metrics.

## Installation

Follow the [instructions](https://docs.halon.io/manual/install.html#installation) in our manual to add our package repository and then run the below command.

### Ubuntu

```
apt-get install halon-extras-prometheus
```

### RHEL

```
yum install halon-extras-prometheus
```

## Configuration

For the configuration schema, see [prometheus.schema.json](prometheus.schema.json).

### smtpd.yaml

```
plugins:
  - id: prometheus
    config:
      address: "127.0.0.1"
      port: 9100
```
