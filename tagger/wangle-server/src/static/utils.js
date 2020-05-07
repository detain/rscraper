"function timestamp2dt(t){"
	"return new Date(t*1000).toISOString().slice(-24, -5)"
"}"
"function wipe_table(selector){"
	"$(selector + \" tr\").remove();"
"}"
"function populate_table(url, selector, postfnct){"
	"$.ajax({"
		"dataType: \"json\","
		"url: url,"
		"success: function(data){"
			"var s = \"\";"
			"for (var row of data){"
				"s += \"<tr>\";"
				"for (var item of row){"
					"s += \"<td>\" + item + \"</td>\";"
				"}"
				"s += \"</tr>\";"
			"}"
			"$(selector).html(s);"
			"if (postfnct !== undefined){"
				"postfnct();"
			"}"
		"},"
		"error: function(){"
			"alert(\"Error populating table\");"
		"}"
	"});"
"}"
"function populate_str(url, selector, postfnct){"
	"$.ajax({"
		"dataType: \"json\","
		"url: url,"
		"success: function(data){"
			"$(selector).html(data.join(\",\"));"
			"if (postfnct !== undefined){"
				"postfnct();"
			"}"
		"},"
		"error: function(){"
			"alert(\"Error populating string\");"
		"}"
	"});"
"}"
"function column_to_permalink(selector, subreddit_indx, link_indx){"
	"$(selector).find('tr').each(function (i, el){"
		"var $tds = $(this).find('td');"
		"var subreddit_name = $tds.eq(subreddit_indx).text();"
		"var $link = $tds.eq(link_indx);"
		"$link.html(\"<a href='https://www.reddit.com/r/\" + subreddit_name + \"/comments/\" + $link.text() + \"'>Link</a>\")"
	"});"
"}"
"function column_from_timestamp(selector, timestamp_indx){"
	"$(selector).find('tr').each(function (i, el){"
		"var $tds = $(this).find('td');"
		"var $link = $tds.eq(timestamp_indx);"
		"$link.value = $link.text();"
		"$link.text(timestamp2dt($link.value));"
	"});"
"}"
"function populate_reasons(){"
	"$.ajax({"
		"dataType: \"json\","
		"url: \"/a/m.json\","
		"success: function(data){"
			"var s = $(\"#m\").html();"
			"for (const [reason_id, reason_name] of Object.entries(data)){"
				"s += \"<option value='\" + reason_id + \"'>\" + reason_name + \"</option>\";"
			"}"
			"$(\"#m\").html(s);"
		"},"
		"error: function(){"
			"alert(\"Error populating table\");"
		"}"
	"});"
"}"
"function sub_into(data, node){"
	"var s = \"\";"
	"for (var tagid of node.text().split(\",\")){"
		"s+=data[tagid]+\" \";"
	"}"
	"node.text(s);"
"}"
"function column_id2name(url, selector, col){"
	"$.ajax({"
		"dataType: \"json\","
		"url: url,"
		"success: function(data){"
			"if (col === undefined){"
				"sub_into(data, $(selector));"
			"} else {"
				"$(selector).find('tr').each(function (i, el){"
					"var $tds = $(this).find('td');"
					"sub_into(data, $tds.eq(col));"
				"});"
			"}"
		"},"
		"error: function(){"
			"alert(\"Error populating table\");"
		"}"
	"});"
"}"
