# Copyright (C) 2003,2004 Nicolas Delon <nicolas@prelude-ids.org>
# All Rights Reserved
#
# This file is part of the Prelude program.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

import sys
import os

import time
import re

import _prelude



class Error(Exception):
    pass



class ClientError(Error):
    def __init__(self, message=""):
        self._message = message

    def __str__(self):
        return self._message



class IDMEFObjectError(Error, KeyError):
    def __init__(self, object):
        self.object = object

    def __str__(self):
        return "Invalid IDMEF object '%s'" % self.object



class IDMEFValueError(Error, ValueError):
    def __init__(self, value, desc=None):
        self.value = value
        self.desc = desc

    def __str__(self):
        if self.desc:
            return "Invalid IDMEF value '%s': %s" % (self.value, self.desc)
        
        return "Invalid IDMEF value '%s'" % self.value



class IDMEFTimeError(Error, ValueError):
    def __init__(self, time):
        self.time = time

    def __str__(self):
        return "Invalid IDMEF time '%s'" % self.time



class IDMEFCriteriaError(Error):
    def __init__(self, criteria):
        self.criteria = criteria

    def __str__(self):
        return "Invalid IDMEF criteria '%s'" % self.criteria



def log_use_syslog():
    _prelude.prelude_log_use_syslog()



class Client:
    RECV_IDMEF = _prelude.PRELUDE_CLIENT_CAPABILITY_RECV_IDMEF
    SEND_IDMEF = _prelude.PRELUDE_CLIENT_CAPABILITY_SEND_IDMEF
    RECV_ADMIN = _prelude.PRELUDE_CLIENT_CAPABILITY_RECV_ADMIN
    SEND_ADMIN = _prelude.PRELUDE_CLIENT_CAPABILITY_SEND_ADMIN
    RECV_CM = _prelude.PRELUDE_CLIENT_CAPABILITY_RECV_CM
    SEND_CM = _prelude.PRELUDE_CLIENT_CAPABILITY_SEND_CM

    def __init__(self, capability, name=None, config=None):
        self._client = None
        self._msgbuf = None
        self._exit_status = _prelude.PRELUDE_CLIENT_EXIT_STATUS_FAILURE
        
        if not name:
            name = sys.argv[0]
        
        self._client = _prelude.prelude_client_new(capability)
        if not self._client:
            raise ClientError()

        #if _prelude.prelude_client_init(self._client, name, config, 1, [ sys.argv[0] ]) < 0:
        if _prelude.prelude_client_init(self._client, name, config, 0, [ ]) < 0:
            raise ClientError()

        if capability & (Client.SEND_IDMEF | Client.SEND_ADMIN | Client.SEND_CM):
            self._msgbuf = _prelude.prelude_msgbuf_new(self._client)
            if not self._msgbuf:
                raise ClientError()

    def set_success(self):
        self._exit_status = _prelude.PRELUDE_CLIENT_EXIT_STATUS_SUCCESS

    def set_failure(self):
        self._exit_status = _prelude.PRELUDE_CLIENT_EXIT_STATUS_FAILURE
        
    def __del__(self):
        if self._msgbuf:
            _prelude.prelude_msgbuf_close(self._msgbuf)
            #self._msgbuf = None

        if self._client:
            _prelude.prelude_client_destroy(self._client, self._exit_status)
            #self._client = None



class Sensor(Client):
    def __init__(self, name, config=None):
        if not config:
            file = os.popen("libprelude-config --prefix")
            path = file.read()
            file.close()
            config = path[:-1] + "/etc/prelude/default/client.conf"
        
        Client.__init__(self, Client.SEND_IDMEF, name, config)
        
        self._analyzer = _prelude.prelude_client_get_analyzer(self._client)
        if not self._analyzer:
            _prelude.prelude_client_destroy(self._client, _prelude.PRELUDE_CLIENT_EXIT_STATUS_FAILURE)
            raise ClientError()

        process = _prelude.idmef_analyzer_get_process(self._analyzer)
        for arg in sys.argv[1:]:
            _prelude.idmef_process_set_arg(process, _prelude.prelude_string_new_dup(arg))
    
    def set_manufacturer(self, manufacturer):
        _prelude.idmef_analyzer_set_manufacturer(self._analyzer, _prelude.prelude_string_new_dup(manufacturer))

    def set_model(self, name, version=None):
        _prelude.idmef_analyzer_set_model(self._analyzer, _prelude.prelude_string_new_dup(name))
        
        if version:
            _prelude.idmef_analyzer_set_version(self._analyzer, _prelude.prelude_string_new_dup(version))

    def set_class(self, class_):
        _prelude.idmef_analyzer_set_class(self._analyzer, _prelude.prelude_string_new_dup(class_))

    def send_alert(self, message):
        alert = _prelude.idmef_message_get_alert(message.res)
        _prelude.idmef_alert_set_analyzer(alert, _prelude.idmef_analyzer_ref(self._analyzer))
        _prelude.idmef_write_message(self._msgbuf, message.res)
        _prelude.prelude_msgbuf_mark_end(self._msgbuf)



class Option:
    def __init__(self, parent, option):
        self.parent = parent
        self.name = _prelude.prelude_option_get_longopt(option)
        self.instantiable = bool(_prelude.prelude_option_get_flags(option) & _prelude.HAVE_CONTEXT)
        self.destroyable = bool(_prelude.prelude_option_get_flags(option) & _prelude.DESTROY_HOOK)
        if self.name.find("[") != -1:
            self.instance = True
            m = re.compile("(.+)\[(.+)\]").match(self.name)
            self.instantiable_name = m.group(1)
            self.instance_name = m.group(2)
        else:
            self.instance = False
        self.boolean = _prelude.prelude_option_get_has_arg(option) == _prelude.no_argument
        self.value = _prelude.prelude_option_get_value(option)
        self.description = _prelude.prelude_option_get_description(option)
        self.options = [ ]
        nodes = [ self.name ]
        while parent:
            nodes.insert(0, parent.name)
            parent = parent.parent
        self.path = ".".join(nodes)

    def find(self, prefix):
        if self.path.find(prefix) == 0:
            return self

        for option in self.options:
            opt = option.find(prefix)
            if opt:
                return opt
            
        return None
        
    def is_section(self):
        return len(self.options) > 0



class Admin(Client):
    def __init__(self, name=None, address="127.0.0.1", port=5554):
        Client.__init__(self, Client.SEND_ADMIN, name, None)
        
        self._manager_connection = _prelude.prelude_connection_new(self._client, address, port)
        if not self._manager_connection:
            raise ClientError("could not create new connection to %s:%p" % (address, port))
        
        if _prelude.prelude_client_set_connection(self._client, self._manager_connection) < 0:
            raise ClientError("could not set client connection")
        
        if _prelude.prelude_connection_connect(self._manager_connection) < 0:
            raise ClientError("could not connect to manager")

    def _get_option_list(self, parent, start):
        options = [ ]
        cur = None
        while True:
            cur = _prelude.prelude_option_get_next(start, cur)
            if not cur:
                break
            
            option = Option(parent, cur)
            options.append(option)
            
            if _prelude.prelude_option_has_optlist(cur):
                option.options = self._get_option_list(option, cur)
                
        return options

    def _request(self, analyzerid, type, value=None):
        _prelude.prelude_option_new_request(self._client, self._msgbuf, 0, analyzerid)
        _prelude.prelude_option_push_request(self._msgbuf, type, value)
        _prelude.prelude_msgbuf_mark_end(self._msgbuf)
        
        msg = _prelude.my_prelude_msg_read(_prelude.prelude_connection_get_fd(self._manager_connection))
        
        return msg
        
    def get_option_list(self, analyzerid):
        msg = self._request(analyzerid, _prelude.PRELUDE_MSG_OPTION_LIST)
        options = _prelude.prelude_option_recv_list(msg)
        
        return self._get_option_list(None, _prelude.prelude_option_get_next(options, None))

    def get_option(self, analyzerid, name):
        pass
    
    def set_option(self, analyzerid, name, value=None):
        if value:
            value = "%s=%s" % (name, value)
        else:
            value = name
        msg = self._request(analyzerid, _prelude.PRELUDE_MSG_OPTION_SET, value)
        retval = _prelude.prelude_option_recv_set(msg)
        
        return retval

    def commit(self, analyzerid, instance):
        msg = self._request(analyzerid, _prelude.PRELUDE_MSG_OPTION_COMMIT, instance)
        if not msg:
            return
        retval = _prelude.prelude_option_recv_set(msg)

        return retval

    def destroy(self, analyzerid, instance):
        msg = self._request(analyzerid, _prelude.PRELUDE_MSG_OPTION_DESTROY, instance)
        if not msg:
            return
        retval = _prelude.prelude_option_recv_set(msg)

        return retval



class IDMEFTime(object):
    """IDMEF time manipulation class."""
    def __init__(self, value=None):
        """__init__([value]) -> idmef time

        value can be:
        int (representing seconds)
        float (representing second . useconds)
        list (with seconds and useconds)
        dict (with 'sec' and 'usec' elements)
        str (a RFC8601 time string)

        if no value is given, the object is built with
        the current time
        
        """
        if value is None:
            value = time.time()
        
        self.res = _idmef_value_time_python_to_c(value)

    def __del__(self):
        try:
            _prelude.idmef_time_destroy(self.res)
        # it can happened if a bad value has been passed to _idmef_value_time_python_to_c
        except AttributeError: 
            pass            
        
    def __str__(self):
        """Return the RFC8601 string representation of the object."""
        buf = "A" * 128

        size = _prelude.idmef_time_to_string(self.res, buf, len(buf))
        if size < 0:
            raise Error()

        return buf[:size]

    def __repr__(self):
        return self.__str__()

    def __int__(self):
        """Return the number of seconds."""
        return _prelude.idmef_time_get_sec(self.res)

    def __float__(self):
        """Return the number of seconds and useconds"""
        sec = float(_prelude.idmef_time_get_sec(self.res))
        usec = float(_prelude.idmef_time_get_usec(self.res))
        
        return sec + usec / 10 ** 6

    def __getattribute__(self, name):
        if name is "sec":
            return _prelude.idmef_time_get_sec(self.res)

        if name is "usec":
            return _prelude.idmef_time_get_usec(self.res)

        if name is "gmt_offset":
            return _prelude.idmef_time_get_gmt_offset(self.res)

        return object.__getattribute__(self, name)


def _idmef_value_time_python_to_c(value):
    time = _prelude.idmef_time_new()
    if not time:
        raise Error()
    
    if type(value) is list:
        try:
            _prelude.idmef_time_set_sec(time, value[0])
            _prelude.idmef_time_set_usec(time, value[1])
        except IndexError:
            _prelude.idmef_time_destroy(time)
            raise
        
    elif type(value) is dict:
        try:
            _prelude.idmef_time_set_sec(time, value['sec'])
            _prelude.idmef_time_set_usec(time, value['usec'])
        except KeyError:
            _prelude.idmef_time_destroy(time)
            raise
    
    elif type(value) is int:
        _prelude.idmef_time_set_sec(time, value)

    elif type(value) is float:
        _prelude.idmef_time_set_sec(time, int(value))
        _prelude.idmef_time_set_usec(time, int(value % 1 * 10 ** 6))
        
    elif type(value) is str:
        if _prelude.idmef_time_set_string(time, value) < 0:
            _prelude.idmef_time_destroy(time);
            raise IDMEFTimeError(value)
        
    else:
        _prelude.idmef_time_destroy(time)
        raise IDMEFTimeError(value)

    return time


def _idmef_integer_python_to_c(object, py_value):
     value_type_table = {
         _prelude.IDMEF_VALUE_TYPE_INT16: { 'py_type': [ int ],
                                'check_value': lambda i: i >= -2 ** 15 and i < 2 ** 15,
                                'convert': _prelude.idmef_value_new_int16 },
        
         _prelude.IDMEF_VALUE_TYPE_UINT16: { 'py_type': [ int ],
                                 'check_value': lambda i: i >= 0 and i < 2 ** 16,
                                 'convert': _prelude.idmef_value_new_uint16 },
        
         _prelude.IDMEF_VALUE_TYPE_INT32: { 'py_type': [ int ],
                                'check': lambda i: i >= -2 ** 31 and i < 2 ** 31,
                                'convert': _prelude.idmef_value_new_int32 },
        
         _prelude.IDMEF_VALUE_TYPE_UINT32: { 'py_type': [ int ],
                                 'check_value': lambda i: i >= 0 and i < 2 ** 32,
                                 'convert': _prelude.idmef_value_new_uint32 },
        
         _prelude.IDMEF_VALUE_TYPE_INT64: { 'py_type': [ long, int ],
                                'check_value': lambda i: i >= -2 ** 63 and i < 2 ** 63,
                                'convert': _prelude.idmef_value_new_int64 },
        
         _prelude.IDMEF_VALUE_TYPE_UINT64: { 'py_type': [ long, int ],
                                 'check_value': lambda i: i >= 0 and i < 2 ** 64,
                                 'convert': _prelude.idmef_value_new_uint64 },
        
         _prelude.IDMEF_VALUE_TYPE_FLOAT: { 'py_type': [ float ],
                                'convert': _prelude.idmef_value_new_float },

         _prelude.IDMEF_VALUE_TYPE_DOUBLE: { 'py_type': [ float ],
                                 'convert': _prelude.idmef_value_new_double },
         }

     object_type = _prelude.idmef_object_get_value_type(object)

     if type(py_value) not in value_type_table[object_type]['py_type']:
         raise IDMEFValueError(py_value, "expected %s, got %s" %
                               (value_type_table[object_type]['py_type'][0], type(py_value)))

     if value_type_table[object_type].has_key('check_value'):
         if not value_type_table[object_type]['check_value'](py_value):
             raise IDMEFValueError(py_value, "out of range")
         
     return value_type_table[object_type]['convert'](py_value)


def _idmef_value_python_to_c(object, py_value):
    object_type = _prelude.idmef_object_get_value_type(object)
    
    if object_type is _prelude.IDMEF_VALUE_TYPE_TIME:
        time = _idmef_value_time_python_to_c(py_value)
        c_value = _prelude.idmef_value_new_time(time)
        if not c_value:
            raise Error()

    elif object_type in [ _prelude.IDMEF_VALUE_TYPE_INT16, _prelude.IDMEF_VALUE_TYPE_UINT16,
                          _prelude.IDMEF_VALUE_TYPE_INT32, _prelude.IDMEF_VALUE_TYPE_UINT32,
                          _prelude.IDMEF_VALUE_TYPE_INT64, _prelude.IDMEF_VALUE_TYPE_UINT64,
                          _prelude.IDMEF_VALUE_TYPE_FLOAT, _prelude.IDMEF_VALUE_TYPE_DOUBLE ]:
        c_value = _idmef_integer_python_to_c(object, py_value)

    elif object_type is _prelude.IDMEF_VALUE_TYPE_ENUM:
        c_value = _prelude.idmef_value_new_enum_string(_prelude.idmef_object_get_type(object), py_value)

    elif object_type is _prelude.IDMEF_VALUE_TYPE_STRING:
        if type(py_value) is not str:
            raise IDMEFValueError(py_value, "expected %s, got %s" % (str, type(py_value)))

        c_string = _prelude.prelude_string_new_dup(py_value)
        if not c_string:
            raise Error()

        c_value = _prelude.idmef_value_new_string(c_string)

    elif object_type is _prelude.IDMEF_VALUE_TYPE_DATA:
        if type(py_value) is not str:
            raise IDMEFValueError(py_value, "expected %s, got %s" % (str, type(py_value)))

        c_data = _prelude.idmef_data_new_dup(py_value, len(py_value) + 1)
        if not c_data:
            raise Error()

        c_value = _prelude.idmef_value_new_data(c_data)

    else: # internal type not recognized/supported
        raise Error()

    if not c_value:
        raise Error()

    return c_value



def idmef_value_c_to_python(value):

    func_type_table = {
        _prelude.IDMEF_VALUE_TYPE_INT8:     _prelude.idmef_value_get_int8,
        _prelude.IDMEF_VALUE_TYPE_UINT8:    _prelude.idmef_value_get_uint8,
        _prelude.IDMEF_VALUE_TYPE_INT16:     _prelude.idmef_value_get_int16,
        _prelude.IDMEF_VALUE_TYPE_UINT16:    _prelude.idmef_value_get_uint16,
        _prelude.IDMEF_VALUE_TYPE_INT32:     _prelude.idmef_value_get_int32,
        _prelude.IDMEF_VALUE_TYPE_UINT32:    _prelude.idmef_value_get_uint32,
        _prelude.IDMEF_VALUE_TYPE_INT64:     _prelude.idmef_value_get_int64,
        _prelude.IDMEF_VALUE_TYPE_UINT64:    _prelude.idmef_value_get_uint64,
        _prelude.IDMEF_VALUE_TYPE_FLOAT:     _prelude.idmef_value_get_float,
        _prelude.IDMEF_VALUE_TYPE_DOUBLE:    _prelude.idmef_value_get_double,
        }

    type = _prelude.idmef_value_get_type(value)

    if type == _prelude.IDMEF_VALUE_TYPE_TIME:
        time = _prelude.idmef_value_get_time(value)
        if not time:
            return None

        py_time = IDMEFTime()
        py_time.res = _prelude.idmef_time_clone(time)
        
        return py_time
    
    if type == _prelude.IDMEF_VALUE_TYPE_STRING:
        string = _prelude.idmef_value_get_string(value)
        if not string:
            return None

        return _prelude.prelude_string_get_string(string)
    
    if type == _prelude.IDMEF_VALUE_TYPE_DATA:
        data = _prelude.idmef_value_get_data(value)
        if not data:
            return None

        return _prelude.idmef_data_get_data(data)

    if type == _prelude.IDMEF_VALUE_TYPE_ENUM:
        return _prelude.idmef_type_enum_to_string(_prelude.idmef_value_get_object_type(value),
                                                  _prelude.idmef_value_get_enum(value))
    
    try:
        func = func_type_table[type]
    except KeyError:
        raise Error()

    return func(value)



def _idmef_value_list_c_to_python(value):
    if value is None:
        return None
    
    if not _prelude.idmef_value_is_list(value):
        return idmef_value_c_to_python(value)

    ret = [ ]

    for i in range(_prelude.idmef_value_get_count(value)):
        ret.append(_idmef_value_list_c_to_python(_prelude.idmef_value_get_nth(value, i)))

    return ret



class IDMEFMessage:
    """IDMEF Message manipulation class."""
    
    def __init__(self, res=None):
        """Create a new empty IDMEF message.""" # FIXME
        if res:
            self.res = res
        else:
            self.res = _prelude.idmef_message_new()
            if not self.res:
                raise Error()

    def __del__(self):
        """Destroy the IDMEF message."""
        _prelude.idmef_message_destroy(self.res)

    def __repr__(self):
        buf = "A" * 8192

        size = _prelude.idmef_message_to_string(self.res, buf, len(buf))
        if size < 0:
            raise Error()

        return buf[:size]

    def __setitem__(self, object_name, py_value):
        """Set the value of the object in the message."""
        object = _prelude.idmef_object_new_fast(object_name)
        if not object:
            raise IDMEFObjectError(object_name)

        try:
            c_value = _idmef_value_python_to_c(object, py_value)
        except Error:
            _prelude.idmef_object_destroy(object)
            raise

        ret = _prelude.idmef_object_set(self.res, object, c_value)
        
        _prelude.idmef_object_destroy(object)
        _prelude.idmef_value_destroy(c_value)

        if ret < 0:
            raise Error()

    def __getitem__(self, object_name):
        """Get the value of the object in the message."""
        object = _prelude.idmef_object_new_fast(object_name)
        if not object:
            raise IDMEFObjectError(object_name)
        
        c_value = _prelude.idmef_object_get(self.res, object)
        _prelude.idmef_object_destroy(object)

        if not c_value:
            return None
        
        try:
            py_value = _idmef_value_list_c_to_python(c_value)
        finally:
            _prelude.idmef_value_destroy(c_value)

        return py_value
        
    def send(self):
        """Send the message to the manager."""
        msgbuf = _prelude.prelude_msgbuf_new(0)
        if not msgbuf:
            raise Error()

        _prelude.idmef_write_message(msgbuf, self.res)
        _prelude.prelude_msgbuf_close(msgbuf)



class IDMEFAlert(IDMEFMessage):
    """IDMEF Alert convenient class, inherit IDMEFMessage."""
    
    def __init__(self):
        """Create a message alert with with some message alert's field already built."""
        IDMEFMessage.__init__(self)
        self["alert.create_time"] = time.time()



class IDMEFCriteria(object):
    "IDMEF criteria manipulation class"

    def __init__(self, criteria_string=None):
        """Create a new idmef criteria.

        The constructor takes a criteria string as argument.
        An empty criteria is built if no argument is given.
        """
        if criteria_string:
            self.res = _prelude.idmef_criteria_new_string(criteria_string)
            if not self.res:
                raise IDMEFCriteriaError(criteria_string)
        else:
            self.res = _prelude.idmef_criteria_new()
            if not self.res:
                raise Error()

    def __del__(self):
        if self.res:
            _prelude.idmef_criteria_destroy(self.res)

    def __str__(self):
        """Return the criteria as a string."""
        buf = "A" * 256

        size = _prelude.idmef_criteria_to_string(self.res, buf, len(buf))
        if size < 0:
            raise Error()

        return buf[:size]

    def __repr__(self):
        self.__str__()

    def __append(self, new_sub_criteria, operator):
        new_criteria = IDMEFCriteria()
        _prelude.idmef_criteria_destroy(new_criteria.res)

        new_criteria.res = _prelude.idmef_criteria_clone(self.res)
        if not new_criteria.res:
            raise Error()

        new_sub_criteria_res = _prelude.idmef_criteria_clone(new_sub_criteria.res)
        if not new_sub_criteria_res:
            raise Error()

        if _prelude.idmef_criteria_add_criteria(new_criteria.res, new_sub_criteria_res, operator) < 0:
            _prelude.idmef_criteria_destroy(new_sub_criteria_res)
            raise Error()

        return new_criteria

    def __and__(self, new_sub_criteria):
        return self.__append(new_sub_criteria, _prelude.operator_and)

    def __or__(self, new_sub_criteria):
        return self.__append(new_sub_criteria, _prelude.operator_or)
