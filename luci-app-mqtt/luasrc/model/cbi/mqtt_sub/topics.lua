m = Map("mqtt_sub")


st = m:section(TypedSection, "topic", translate("Topics"), translate("") )
st.addremove = true
st.anonymous = true
st.template = "mqtt/tblsection2"
st.novaluetext = "There are no topics created yet."

topic = st:option(Value, "topic", translate("Topic name"), translate(""))
topic.datatype = "string"
topic.maxlength = 65536
topic.placeholder = translate("Topic")
topic.rmempty = false
topic.parse = function(self, section, novld, ...)
	local value = self:formvalue(section)
	if value == nil or value == "" then
		self.map:error_msg(translate("Topic name can not be empty"))
		self.map.save = false
	end
	Value.parse(self, section, novld, ...)
end

retained = st:option(ListValue, "want_retained", translate("Retained"), translate("What to do with retained messages"))
retained:value("1", "accept")
retained:value("0", "reject")
retained.default = "1"

qos = st:option(ListValue, "qos", translate("QoS level"), translate("The publish/subscribe QoS level used for this topic"))
qos:value("0", "At most once (0)")
qos:value("1", "At least once (1)")
qos:value("2", "Exactly once (2)")
qos.rmempty=false
qos.default="0"



    return m
    
