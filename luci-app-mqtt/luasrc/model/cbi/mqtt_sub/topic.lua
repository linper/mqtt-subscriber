local dsp = require "luci.dispatcher"
local util  = require "luci.util"
local uci = require("luci.model.uci").cursor()
local s, o

m = Map("mqtt_topics")
local sid = arg[1]
m.redirect = dsp.build_url("admin", "services", "mqtt", "subscriber", "topics")
s = m:section(NamedSection, sid, "topic", translate("Topic configuration"))
s.addremove = true
s.anonymous = true

if not m.uci:get("mqtt_topics", sid) then
	luci.http.redirect(m.redirect)
end

o = s:option(Flag, "enabled", translate('Enable'), translate("Enable topic instance"))

o = s:option(Value, "topic", translate("Topic name"), translate(""))
t_name = o
o.datatype = "string"
o.maxlength = 65536
o.placeholder = translate("Topic")
o.rmempty = false
o.parse = function(self, section, novld, ...)
	local value = self:formvalue(section)
	if value == nil or value == "" then
		self.map:error_msg(translate("Topic name can not be empty"))
		self.map.save = false
	end
	Value.parse(self, section, novld, ...)
end

o = s:option(ListValue, "qos", translate("QoS level"), translate("The publish/subscribe QoS level used for this topic"))
o:value("0", "At most once (0)")
o:value("1", "At least once (1)")
o:value("2", "Exactly once (2)")
o.rmempty=false
o.default="0"

constrain = s:option(Flag, "constrain", translate('Constrain data'), translate("Provide datatypes to constrain received."))

o = s:option(DynamicList, "type", translate("Allowed datatypes"))
o:depends("constrain", "1")
o.datatype = "string"


return m
	
	