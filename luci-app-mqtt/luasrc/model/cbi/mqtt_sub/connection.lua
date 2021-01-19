m = Map("mqtt_sub", translate("MQTT Subscriber Connection Info"), translate("An MQTT Subscriber is a client that receives published messages. "))
local certs = require "luci.model.certificate"
local s = m:section(NamedSection, "mqtt_sub", "mqtt_sub",  translate(""), translate(""))

enabled_sub = s:option(Flag, "enabled", translate("Enable"), translate("Select to enable MQTT subscriber"))

remote_addr = s:option(Value, "remote_addr", translate("Hostname"), translate("Specify address of the broker"))
remote_addr:depends("enabled", "1")
remote_addr.placeholder  = "www.example.com"
remote_addr.datatype = "host"
remote_addr.parse = function(self, section, novld, ...)
	local enabled = luci.http.formvalue("cbid.mqtt_sub.mqtt_sub.enabled")
	local value = self:formvalue(section)
	if enabled and (value == nil or value == "") then
		self:add_error(section, "invalid", "Error: hostname is empty")
	end
	Value.parse(self, section, novld, ...)
end

remote_port = s:option(Value, "remote_port", translate("Port"), translate("Specify port of the broker"))
remote_port:depends("enabled", "1")
remote_port.default = "1883"
remote_port.placeholder = "1883"
remote_port.datatype = "port"
remote_port.parse = function(self, section, novld, ...)
	local enabled = luci.http.formvalue("cbid.mqtt_sub.mqtt_sub.enabled")
	local value = self:formvalue(section)
	if enabled and (value == nil or value == "") then
		self:add_error(section, "invalid", "Error: port is empty")
	end
	Value.parse(self, section, novld, ...)
end

remote_username = s:option(Value, "username", "Username", "Specify username of remote host")
remote_username.datatype = "credentials_validate"
remote_username.placeholder = translate("Username")
remote_username:depends("enabled", "1")
remote_username.parse = function(self, section, novld, ...)
	local enabled = luci.http.formvalue("cbid.mqtt_sub.mqtt_sub.enabled")
	local pass = luci.http.formvalue("cbid.mqtt_sub.mqtt_sub.password")
	local value = self:formvalue(section)
	if enabled and pass ~= nil and pass ~= "" and (value == nil or value == "") then
		self:add_error(section, "invalid", "Error: username is empty but password is not")
	end
	Value.parse(self, section, novld, ...)
end

remote_password = s:option(Value, "password", translate("Password"), translate("Specify password of remote host. Allowed characters (a-zA-Z0-9!@#$%&*+-/=?^_`{|}~. )"))
remote_password:depends("enabled", "1")
remote_password.password = true
remote_password.datatype = "credentials_validate"
remote_password.placeholder = translate("Password")
remote_password.parse = function(self, section, novld, ...)
	local enabled = luci.http.formvalue("cbid.mqtt_sub.mqtt_sub.enabled")
	local user = luci.http.formvalue("cbid.mqtt_sub.mqtt_sub.username")
	local value = self:formvalue(section)
	if enabled and user ~= nil and user ~= "" and (value == nil or value == "") then
		self:add_error(section, "invalid", "Error: password is empty but username is not")
	end
	Value.parse(self, section, novld, ...)
end

FileUpload.size = "262144"
FileUpload.sizetext = translate("Selected file is too large, max 256 KiB")
FileUpload.sizetextempty = translate("Selected file is empty")
FileUpload.unsafeupload = true

tls_enabled = s:option(Flag, "tls", "TLS", "Select to enable TLS encryption")
tls_enabled:depends("enabled", "1")

tls_insecure = s:option(Flag, "tls_insecure", translate("Allow insecure connection"), translate("Allow not verifying server authenticity"))
tls_insecure:depends({enabled = "1", tls = "1"})

local certificates_link = luci.dispatcher.build_url("admin", "system", "admin", "certificates")
o = s:option(Flag, "_device_files", translate("Certificate files from device"), translatef("Choose this option if you want to select certificate files from device.\
																					Certificate files can be generated <a class=link href=%s>%s</a>", certificates_link, translate("here")))
o:depends("tls", "1")
local cas = certs.get_ca_files().certs
local certificates = certs.get_certificates()
local keys = certs.get_keys()

tls_cafile = s:option(FileUpload, "cafile", "CA file", "")
tls_cafile:depends({tls = "1", _device_files=""})

tls_certfile = s:option(FileUpload, "certfile", "Certificate file", "")
tls_certfile:depends({tls = "1", _device_files=""})

tls_keyfile = s:option(FileUpload, "keyfile", "Key file", "")
tls_keyfile:depends({tls = "1", _device_files=""})

tls_cafile = s:option(ListValue, "_device_cafile", "CA file", "")
tls_cafile:depends({tls = "1", _device_files="1"})

if #cas > 0 then
	for _,ca in pairs(cas) do
		tls_cafile:value("/etc/certificates/" .. ca.name, ca.name)
	end
else 
	tls_cafile:value("", translate("-- No files available --"))
end

function tls_cafile.write(self, section, value)
	m.uci:set(self.config, section, "cafile", value)
end

tls_cafile.cfgvalue = function(self, section)
	return m.uci:get(m.config, section, "cafile") or ""
end

tls_certfile = s:option(ListValue, "_device_certfile", "Certificate file", "")
tls_certfile:depends({tls = "1", _device_files="1"})

if #certificates > 0 then
	for _,certificate in pairs(certificates) do
		tls_certfile:value("/etc/certificates/" .. certificate.name, certificate.name)
	end
else 
	tls_certfile:value("", translate("-- No files available --"))
end

function tls_certfile.write(self, section, value)
	m.uci:set(self.config, section, "certfile", value)
end

tls_certfile.cfgvalue = function(self, section)
	return m.uci:get(m.config, section, "certfile") or ""
end

tls_keyfile = s:option(ListValue, "_device_keyfile", "Key file", "")
tls_keyfile:depends({tls = "1", _device_files="1"})

if #keys > 0 then
	for _,key in pairs(keys) do
		tls_keyfile:value("/etc/certificates/" .. key.name, key.name)
	end
else 
	tls_keyfile:value("", translate("-- No files available --"))
end

function tls_keyfile.write(self, section, value)
	m.uci:set(self.config, section, "keyfile", value)
end

tls_keyfile.cfgvalue = function(self, section)
	return m.uci:get(m.config, section, "keyfile") or ""
end

return m
