function v = HELICS_DATA_TYPE_RAW()
  persistent vInitialized;
  if isempty(vInitialized)
    vInitialized = helicsMEX(0, 1398230928);
  end
  v = vInitialized;
end
