function v = helics_state_pending_exec()
  persistent vInitialized;
  if isempty(vInitialized)
    vInitialized = helicsMEX(0, 1593856881);
  end
  v = vInitialized;
end
