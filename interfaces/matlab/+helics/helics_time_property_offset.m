function v = helics_time_property_offset()
  persistent vInitialized;
  if isempty(vInitialized)
    vInitialized = helicsMEX(0, 1398230899);
  end
  v = vInitialized;
end
