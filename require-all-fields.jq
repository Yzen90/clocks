walk(
  if type == "object" and has("properties") then 
    .required = ( .properties | keys ) 
  else 
    . end
)
