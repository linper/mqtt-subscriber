local dsp = require "luci.dispatcher"
local util  = require "luci.util"
local uci = require("luci.model.uci").cursor()
local s, o

m = Map("mqtt_sub")
local sid = arg[1]
m.redirect = dsp.build_url("admin", "services", "mqtt", "subscriber", "events")
s = m:section(NamedSection, sid, "event", translate("Event configuration"))
s.addremove = true
s.anonymous = true

if not m.uci:get("mqtt_sub", sid) then
	luci.http.redirect(m.redirect)
end

o = s:option(Flag, "enabled", translate('Enable'), translate("Enable topic instance"))

o = s:option(ListValue, "t_id", translate("Topic"))
m.uci:foreach("mqtt_sub", "topic", function(s)
	o:value(s.id, s.topic)
end)

o = s:option(ListValue, "datatype", translate("Data type"))
o:value("0", translate("Integer"))
o:value("1", translate("String"))

o = s:option(Value, "field", translate("Data field"), translate("Data of this datafield will be compared with target"))
o.datatype = "string"
o.maxlength = 65536
o.placeholder = translate("Data field")
o.rmempty = false
o.parse = function(self, section, novld, ...)
	local value = self:formvalue(section)
	if value == nil or value == "" then
		self.map:error_msg(translate("Data field can not be empty"))
		self.map.save = false
	end
	Value.parse(self, section, novld, ...)
end

o = s:option(ListValue, "rule", translate("comparison rule"))
o.rmempty = false
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
o.rmempty = false
o.parse = function(self, section, novld, ...)
	local value = self:formvalue(section)
	if value == nil or value == "" then
		self.map:error_msg(translate("Target can not be empty"))
		self.map.save = false
	end
	Value.parse(self, section, novld, ...)
end

return m