contract test {
  /// @return returns something
  uint private state;
}
// ----
// DocstringParsingError 9440: (18-47): Documentation tag "@return" is only allowed on public state-variables.
