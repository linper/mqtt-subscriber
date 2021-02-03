local uci = require("luci.model.uci").cursor()
local dsp = require "luci.dispatcher"
local m ,s, o

m = Map("mqtt_events")

se = m:section(TypedSection, "event", translate("Events"))
se.addremove = true
se.anonymous = true
se.template = "cbi/tblsection"
se.novaluetext = "There are no events created yet."
se.main_toggled = true
se.extedit = dsp.build_url("admin", "services", "mqtt", "subscriber", "events", "%s")


function se.create(self)
	stat = TypedSection.create(self, name)
	uci:set(self.config, stat, "t_id", "-1")
	uci:set(self.config, stat, "enabled", "0")
	luci.http.redirect(dsp.build_url("admin", "services", "mqtt", "subscriber",  "events", stat))
	return stat
end

o = se:option( DummyValue, "t_id", translate("Topic"))
o.datatype = "string"
function o.cfgvalue(self, section)
	local id = m.uci:get("mqtt_events", section, "t_id")
	local found = "0"
	self.map.uci:foreach("mqtt_topics", "topic", function(s)
		if id == s.id then
			found = s.topic
		end
    end)
	return found
end

o = se:option( DummyValue, "field", translate("Data field"))
o.datatype = "string"

local c_tbl = {}
c_tbl[1] = "equal to"
c_tbl[2] = "not equal to"
c_tbl[3] = "more than"
c_tbl[4] = "less than"
c_tbl[5] = "more or equal to"
c_tbl[6] = "less or equal to"
o = se:option( DummyValue, "rule", translate("Comparison rule"))
o.datatype = "string"
function o.cfgvalue(self, section)
	local r_id = tonumber(self.map:get(section, "rule"))
	return c_tbl[r_id]
end
o = se:option( DummyValue, "target", translate("Comparison target"))
o.datatype = "string"

se:option(Flag, "enabled")

return m

