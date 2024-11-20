PROJECT_NAME = duck-query-lambda
OUTPUT_DIR = output
EXTENSIONS = httpfs
DUCKDB_VERSION = 1.1.3

.PHONY: docker_build binary build-bootstrap build-DuckFunction build-DuckFunctionArm

docker_build:
	@echo Building Docker Image for $(ARCH)
	docker build --platform=linux/$(ARCH) -t $(PROJECT_NAME)_$(ARCH) .

binary: docker_build
	mkdir -p $(OUTPUT_DIR)/$(ARCH)
	docker run -v /tmp:/$(OUTPUT_DIR) $(PROJECT_NAME)_$(ARCH) sh -c \
		'cp /usr/local/lib/libduckdb* . && strip libduckdb*.so $(PROJECT_NAME)* && tar cf - libduckdb*.so $(PROJECT_NAME)* | base64' \
		> $(OUTPUT_DIR)/artifacts_$(ARCH).b64
	cat $(OUTPUT_DIR)/artifacts_$(ARCH).b64 | base64 -d | tar xf - -C $(OUTPUT_DIR)/$(ARCH)

build-bootstrap: binary
	@echo Creating bootstrap artifacts in $(ARTIFACTS_DIR)
	cp $(OUTPUT_DIR)/$(ARCH)/lib* $(ARTIFACTS_DIR)
	cp $(OUTPUT_DIR)/$(ARCH)/$(PROJECT_NAME) $(ARTIFACTS_DIR)/bootstrap
	cp $(OUTPUT_DIR)/$(ARCH)/$(PROJECT_NAME)_test $(ARTIFACTS_DIR)/test_main
	chmod +x $(ARTIFACTS_DIR)/bootstrap
	chmod +x $(ARTIFACTS_DIR)/test_main

download-extensions:
	@echo "Downloading DuckDB extensions..."
	mkdir -p $(ARTIFACTS_DIR)/duckdb_extensions/v$(DUCKDB_VERSION)/linux_$(ARCH)/
	$(foreach ext, $(EXTENSIONS), \
		curl -sfL "http://extensions.duckdb.org/v$(DUCKDB_VERSION)/linux_$(ARCH)/$(ext).duckdb_extension.gz" | gunzip - > "$(ARTIFACTS_DIR)/duckdb_extensions/v$(DUCKDB_VERSION)/linux_$(ARCH)/$(ext).duckdb_extension"; \
	)

build-LambdaLayerArm64:
	@echo Building for Arm
	$(MAKE) build-bootstrap ARCH=arm64
	$(MAKE) download-extensions ARCH=arm64

build-LambdaLayerX8664:
	@echo Building for x86_64
	$(MAKE) build-bootstrap ARCH=x86_64
	$(MAKE) download-extensions ARCH=x86_64
