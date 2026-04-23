scada_register_package(
  NAME scada-common
  REPO common
  COMPONENT Common
  DISPLAY_NAME "Общие компоненты"
  VERSION "${SCADA_PACKAGING_VERSION}"
  DESCRIPTION "Shared runtime package"
  DISTRIBUTION_KIND "dependency"
  STANDALONE_INSTALLER_DEFAULT OFF
  INSTALL_DESTINATIONS "bin"
  RUNTIME_DEPENDENCY_POLICY "repo-local-runtime-deps"
  DEPENDS scada-core)
