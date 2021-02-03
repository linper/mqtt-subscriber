local dsp = require "luci.dispatcher"
local util  = require "luci.util"
local uci = require("luci.model.uci").cursor()
local s, o

m = Map("mqtt_events")
local sid = arg[1]
m.redirect = dsp.build_url("admin", "services", "mqtt", "subscriber", "events")
s = m:section(NamedSection, sid, "event", translate("Event configuration"))
s.addremove = true
s.anonymous = true

if not m.uci:get("mqtt_events", sid) then
	luci.http.redirect(m.redirect)
end

o = s:option(Flag, "enabled", translate('Enable'), translate("Enable topic instance"))

o = s:option(ListValue, "t_id", translate("Topic"))
m.uci:foreach("mqtt_topics", "topic", function(s)
	o:value(s.id, s.topic)
end)

dtype = s:option(ListValue, "datatype", translate("Data type"))
dtype:value("0", translate("Integer"))
dtype:value("1", translate("Double"))
dtype:value("2", translate("String"))

o = s:option(Value, "field", translate("Data field"), translate("Data of this datafield will be compared with target"))
o.datatype = "string"
o.maxlength = 65536
o.placeholder = translate("Data field")
o.parse = function(self, section, novld, ...)
	local value = self:formvalue(section)
	if (value == nil or value == "") then
		self:add_error(section, "invalid", translate("Error: data field can not be empty"))
		self.map.save = false
	end
	Value.parse(self, section, novld, ...)
end

o = s:option(ListValue, "rule", translate("Comparison rule"))
o.default = "1"
o:value("1", "equal to")
o:value("2", "not equal to")
o:value("3", "more than")
o:value("4", "less than")
o:value("5", "more or equal to")
o:value("6", "less or equal to")

o = s:option(Value, "target", translate("Target"), translate("Comparison target(value to compare to)"))
o.datatype = "string"
o.maxlength = 65536
o.placeholder = translate("Target")
o.parse = function(self, section, novld, ...)
	local value = self:formvalue(section)
	local dtt = dtype:formvalue(section)
	local t = self.map:get(section, "target")
	if value == nil or value == "" then
		self:add_error(section, "invalid", translate("Error: target can not be empty"))
		self.map.save = false
	elseif dtt ~= "2" and tonumber(value, 10) == nil then
		self:add_error(section, "invalid", translate("Error: target is not right data type"))
		self.map.save = false
	end
	Value.parse(self, section, novld, ...)
end

o = s:option(Flag, "en_interv", translate('Enable event interval'), translate("Enambe minimum event interval"))

o = s:option(Value, "interval", translate('Event interval'), translate("Set minimum amount of seconds between event invocations"))
o:depends("en_interv", "1")
o.datatype = "uinteger"
o.placeholder = "300"
o.parse = function(self, section, novld, ...)
	local enabled_interv = luci.http.formvalue("cbid.mqtt_events."..section..".en_interv")
	local value = self:formvalue(section)
	local dtt = dtype:formvalue(section)
	if enabled_interv and (value == nil or value == "") then
		self:add_error(section, "invalid", translate("Error: even interval can not be empty"))
		self.map.save = false
	elseif enabled_interv and tonumber(value, 10) < 10  then
		self:add_error(section, "invalid", translate("Error: interval nust be number not lower than 10"))
		self.map.save = false
	end
	Value.parse(self, section, novld, ...)
end

local is_group = false
mailGroup = s:option(ListValue, "emailgroup", translate("Email account"), translate("Recipient's email configuration <br/>(<a href=\"/cgi-bin/luci/admin/system/admin/group/email\" class=\"link\">configure it here</a>)"))
m.uci:foreach("user_groups", "email", function(s)
	if s.senderemail then
		mailGroup:value(s.name, s.name)
		is_group = true
	end
end)
if not is_group then
	mailGroup:value(0, translate("No email accounts created"))
end

function mailGroup.parse(self, section, novld, ...)
	local val = self:formvalue(section)
	if val and val == "0" then
		self:add_error(section, "invalid", translate("Error: no email accounts selected"))
	end
	Value.parse(self, section, novld, ...)
end

recEmail = s:option(DynamicList, "recip", translate("Recipient's email address"), translate("For whom you want to send an email to. Allowed characters (a-zA-Z0-9._%+@-)"))
recEmail.datatype = "email"
recEmail.placeholder = "mail@domain.com"
recEmail.rmempty = false

function m.on_save(self)
	local group_name = m:formvalue("cbid.mqtt_events."..arg[1]..".emailgroup")
	local group
	m.uci:foreach("user_groups", "email", function(s)
		if s.name == group_name then
			group = s[".name"]
		end
	end)
	local smtpIP = m.uci:get("user_groups", group, "smtp_ip") 
	local smtpPort = m.uci:get("user_groups", group, "smtp_port") 
	local username = m.uci:get("user_groups", group, "username") 
	local passwd = m.uci:get("user_groups", group, "password") 
	local senderEmail = m.uci:get("user_groups", group, "senderemail") 
	local secure = m.uci:get("user_groups", group, "secure_conn") 
	m.uci:set("mqtt_events",arg[1],"sender_email", senderEmail)
	m.uci:set("mqtt_events",arg[1],"password", passwd)
	m.uci:set("mqtt_events",arg[1],"username", username)
	m.uci:set("mqtt_events",arg[1],"smtp_port", smtpPort)
	m.uci:set("mqtt_events",arg[1],"smtp_ip", smtpIP)
	m.uci:set("mqtt_events",arg[1],"secure_conn", secure)
end

return m