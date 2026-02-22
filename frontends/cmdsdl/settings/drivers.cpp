auto DriverSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

}

auto DriverSettings::videoRefresh() -> void {
  videoDriverList.reset();
  videoMonitorList.setEnabled(videoMonitorList.itemCount() > 1);
  VerticalLayout::resize();
}

auto DriverSettings::videoDriverUpdate() -> void {
  program.videoDriverUpdate();
  videoRefresh();
}

auto DriverSettings::audioDriverUpdate() -> void {

  program.audioDriverUpdate();
}

auto DriverSettings::inputRefresh() -> void {
  inputDriverList.reset();
}

auto DriverSettings::inputDriverUpdate() -> void {

  program.inputDriverUpdate();
  inputRefresh();
}
