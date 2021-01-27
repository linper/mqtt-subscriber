local dsp = require "luci.dispatcher"
local util  = require "luci.util"
local uci = require("luci.model.uci").cursor()
local s, o

m = Map("mqtt_ev")
local sid = arg[1]
m.redirect = dsp.build_url("admin", "services", "mqtt", "subscriber", "events")
s = m:section(NamedSection, sid, "event", translate("Event configuration"))
s.addremove = true
s.anonymous = true

if not m.uci:get("mqtt_ev", sid) then
	luci.http.redirect(m.redirect)
end

enabl = s:option(Flag, "enabled", translate('Enable'), translate("Enable topic instance"))

o = s:option(ListValue, "t_id", translate("Topic"))
o:depends("enabled", "1")
m.uci:foreach("mqtt_sub", "topic", function(s)
	o:value(s.id, s.topic)
end)

dtype = s:option(ListValue, "datatype", translate("Data type"))
dtype:depends("enabled", "1")
dtype:value("0", translate("Integer"))
dtype:value("1", translate("Double"))
dtype:value("2", translate("String"))

o = s:option(Value, "field", translate("Data field"), translate("Data of this datafield will be compared with target"))
o:depends("enabled", "1")
o.datatype = "string"
o.maxlength = 65536
o.placeholder = translate("Data field")
o.parse = function(self, section, novld, ...)
	local enabled = luci.http.formvalue("cbid.mqtt_ev."..section..".enabled")
	local value = self:formvalue(section)
	if enabled and (value == nil or value == "") then
		self:add_error(section, "invalid", translate("Error: data field can not be empty"))
		self.map.save = false
	end
	Value.parse(self, section, novld, ...)
end

o = s:option(ListValue, "rule", translate("comparison rule"))
o:depends("enabled", "1")
o.rmempty = false
o:value("1", "equal to")
o:value("2", "not equal to")
o:value("3", "more than")
o:value("4", "less than")
o:value("5", "more or equal to")
o:value("6", "less or equal to")

o = s:option(Value, "target", translate("Target"), translate("Comparison target(value to compare to)"))
o:depends("enabled", "1")
o.datatype = "string"
o.maxlength = 65536
o.placeholder = translate("Target")
o.parse = function(self, section, novld, ...)
	local enabled = luci.http.formvalue("cbid.mqtt_ev."..section..".enabled")
	local value = self:formvalue(section)
	local dtt = dtype:formvalue(section)
	local t = self.map:get(section, "target")
	if enabled then
		if value == nil or value == "" then
			self.map:error_msg(translate("Target can not be empty"))
			self.map.save = false
		elseif dtt ~= "2" and tonumber(value, 10) == nil then
			self:add_error(section, "invalid", translate("Error: target is not right data type"))
			self.map.save = false
		end
	end
	Value.parse(self, section, novld, ...)
end

username = s:option(Value, "username", "Sender e-mail", "Specify sender email")
username.datatype = "credentials_validate"
username.placeholder = translate("Sender e-mail")
username:depends("enabled", "1")
username.parse = function(self, section, novld, ...)
	local enabled = luci.http.formvalue("cbid.mqtt_ev."..section..".enabled")
	local pass = luci.http.formvalue("cbid.mqtt_ev."..section..".password")
	local value = self:formvalue(section)
	if enabled and pass ~= nil and pass ~= "" and (value == nil or value == "") then
		self:add_error(section, "invalid", "Error: username is empty but password is not")
	end
	Value.parse(self, section, novld, ...)
end

password = s:option(Value, "password", translate("Sender password"), translate("Specify sender e-mail password. Allowed characters (a-zA-Z0-9!@#$%&*+-/=?^_`{|}~. )"))
password:depends("enabled", "1")
password.password = true
password.datatype = "credentials_validate"
password.placeholder = translate("Password")
password.parse = function(self, section, novld, ...)
	local enabled = luci.http.formvalue("cbid.mqtt_ev."..section..".enabled")
	local pass = luci.http.formvalue("cbid.mqtt_ev."..section..".username")
	local value = self:formvalue(section)
	if enabled and user ~= nil and user ~= "" and (value == nil or value == "") then
		self:add_error(section, "invalid", "Error: password is empty but username is not")
	end
	Value.parse(self, section, novld, ...)
end

mail_srv = s:option(Value, "mail_srv", translate("Mail server"), translate("specify mail server url, with port included"))
mail_srv:depends("enabled", "1")
mail_srv.placeholder  = "smtps://smtp.gmail.com:465"
mail_srv.datatype = "string"
mail_srv.maxlength = 4096
mail_srv.parse = function(self, section, novld, ...)
	local enabled = luci.http.formvalue("cbid.mqtt_ev."..section..".enabled")
	local value = self:formvalue(section)
	if enabled and (value == nil or value == "") then
		self:add_error(section, "invalid", "Error: mail server is empty")
	end
	Value.parse(self, section, novld, ...)
end

receiver = s:option(Value, "receiver", "Receiver e-mail", "Specify receiver email")
receiver.datatype = "string"
receiver.placeholder = translate("Receiver e-mail")
receiver:depends("enabled", "1")
o.rmempty = false
receiver.maxlength = 4096
receiver.parse = function(self, section, novld, ...)
	local enabled = luci.http.formvalue("cbid.mqtt_ev."..section..".enabled")
	local value = self:formvalue(section)
	if enabled and (value == nil or value == "") then
		self:add_error(section, "invalid", "Error: receiver e-mail is empty")
	end
	Value.parse(self, section, novld, ...)
end

return m