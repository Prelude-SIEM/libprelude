#include <iostream>

#include <string.h>
#include <prelude.h>
#include <idmef-path.h>
#include <idmef-message-print.h>
#include <idmef-object-prv.h>

#include "prelude-error.hxx"
#include "idmef-path.hxx"
#include "idmef.hxx"

using namespace Prelude;


IDMEF::IDMEF() : _object(NULL)
{
        int ret;

        ret = idmef_message_new((idmef_message_t **) &_object);
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEF::IDMEF(const IDMEF &idmef)
{
        _object = (idmef._object) ? idmef_object_ref(idmef._object) : NULL;
}


IDMEF::IDMEF(idmef_object_t *object)
{
        _object = object;
}


IDMEF::~IDMEF()
{
        if ( _object )
                idmef_object_destroy(_object);
}

void IDMEF::Set(const char *path, std::vector<IDMEF> value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


void IDMEF::Set(const char *path, IDMEF *value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


void IDMEF::Set(const char *path, std::vector<IDMEFValue> value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


void IDMEF::Set(const char *path, IDMEFValue *value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


void IDMEF::Set(const char *path, IDMEFValue &value)
{
        IDMEFPath(*this, path).Set(*this, &value);
}


void IDMEF::Set(const char *path, std::string value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


void IDMEF::Set(const char *path, const char *value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


void IDMEF::Set(const char *path, int8_t value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


void IDMEF::Set(const char *path, uint8_t value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


void IDMEF::Set(const char *path, int16_t value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


void IDMEF::Set(const char *path, uint16_t value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


void IDMEF::Set(const char *path, int32_t value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


void IDMEF::Set(const char *path, uint32_t value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


void IDMEF::Set(const char *path, int64_t value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


void IDMEF::Set(const char *path, uint64_t value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


void IDMEF::Set(const char *path, float value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


void IDMEF::Set(const char *path, double value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


void IDMEF::Set(const char *path, IDMEFTime &value)
{
        IDMEFPath(*this, path).Set(*this, value);
}


IDMEFValue IDMEF::Get(const char *path)
{
        return IDMEFPath(*this, path).Get(*this);
}


int IDMEF::operator == (const IDMEF *idmef)
{
        if ( ! idmef )
                return 0;

        if ( _object == idmef->_object )
                return 1;

        return idmef_object_compare(_object, idmef->_object) == 0 ? 1 : 0;
}


IDMEF IDMEF::Clone()
{
        int ret;
        idmef_object_t *clone;

        ret = idmef_object_clone(_object, &clone);
        if ( ret < 0 )
                throw PreludeError(ret);

        return IDMEF(clone);
}


idmef_class_id_t IDMEF::GetId() const
{
        return _object->_idmef_object_id;
}


const std::string IDMEF::ToString() const
{
        int ret;
        std::string str;
        prelude_io_t *fd;

        ret = prelude_io_new(&fd);
        if ( ret < 0 )
                throw PreludeError(ret);

        prelude_io_set_buffer_io(fd);
        idmef_object_print(_object, fd);

        str.assign((const char *) prelude_io_get_fdptr(fd), prelude_io_pending(fd));

        prelude_io_close(fd);
        prelude_io_destroy(fd);

        return str;
}



static int ostream_write(prelude_msgbuf_t *fd, prelude_msg_t *msg)
{
        std::ostream *os = (std::ostream *) prelude_msgbuf_get_data(fd);

        os->write((const char *)prelude_msg_get_message_data(msg), prelude_msg_get_len(msg));
        prelude_msg_recycle(msg);

        return 0;
}


static ssize_t istream_read(prelude_io_t *fd, void *buf, size_t size)
{
        std::istream *is = (std::istream *) prelude_io_get_fdptr(fd);

        is->read((char *) buf, size);
        return is->gcount();
}



void IDMEF::_genericRead(ssize_t (read_cb)(prelude_io_t *fd, void *buf, size_t size), void *fd_data)
{
        int ret;
        prelude_io_t *fd;
        prelude_msg_t *msg = NULL;

        if ( this->_object->_idmef_object_id != IDMEF_CLASS_ID_MESSAGE )
                throw PreludeError("read operation only supported on root IDMEF object");

        ret = prelude_io_new(&fd);
        if ( ret < 0 )
                throw PreludeError(ret);

        prelude_io_set_fdptr(fd, fd_data);
        prelude_io_set_read_callback(fd, read_cb);

        ret = prelude_msg_read(&msg, fd);
        prelude_io_destroy(fd);
        if ( ret < 0 )
                throw PreludeError(ret);

        ret = idmef_message_read((idmef_message_t *) this->_object, msg);
        if ( ret < 0 ) {
                prelude_msg_destroy(msg);
                throw PreludeError(ret);
        }

        idmef_message_set_pmsg((idmef_message_t *) this->_object, msg);
}


std::istream &operator >>(std::istream &is, Prelude::IDMEF &idmef)
{
        idmef._genericRead(istream_read, &is);
        return is;
}



size_t IDMEF::_genericWrite(int (write_cb)(prelude_msgbuf_t *msgbuf, prelude_msg_t *msg), void *fd_data) const
{
        int ret;
        prelude_msgbuf_t *fd;

        if ( this->_object->_idmef_object_id != IDMEF_CLASS_ID_MESSAGE )
                throw PreludeError("write operation only supported on root IDMEF object");

        ret = prelude_msgbuf_new(&fd);
        if ( ret < 0 )
                throw PreludeError(ret);

        prelude_msgbuf_set_data(fd, fd_data);
        prelude_msgbuf_set_callback(fd, write_cb);

        ret = idmef_message_write((idmef_message_t *) this->_object, fd);
        if ( ret < 0 ) {
                prelude_msgbuf_destroy(fd);
                throw PreludeError(ret);
        }

        prelude_msgbuf_destroy(fd);
        return ret;
}


std::ostream &operator <<(std::ostream &os, const IDMEF &idmef)
{
        idmef._genericWrite(ostream_write, &os);
        return os;
}


IDMEF::operator const std::string() const
{
        return ToString();
}


IDMEF::operator idmef_object_t *() const
{
        return (idmef_object_t *) _object;
}


IDMEF &IDMEF::operator = (const IDMEF &idmef)
{
        if ( this != &idmef && _object != idmef._object ) {
                if ( _object )
                        idmef_object_destroy(_object);

                _object = (idmef._object) ? idmef_object_ref(idmef._object) : NULL;
        }

        return *this;
}
