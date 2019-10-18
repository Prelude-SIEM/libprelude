# This file was automatically generated by SWIG (http://www.swig.org).
# Version 4.0.1
#
# Do not make changes to this file unless you know what you are doing--modify
# the SWIG interface file instead.

package Prelude;
use base qw(Exporter);
use base qw(DynaLoader);
package Preludec;
bootstrap Prelude;
package Prelude;
@EXPORT = qw();

# ---------- BASE METHODS -------------

package Prelude;

sub TIEHASH {
    my ($classname,$obj) = @_;
    return bless $obj, $classname;
}

sub CLEAR { }

sub FIRSTKEY { }

sub NEXTKEY { }

sub FETCH {
    my ($self,$field) = @_;
    my $member_func = "swig_${field}_get";
    $self->$member_func();
}

sub STORE {
    my ($self,$field,$newval) = @_;
    my $member_func = "swig_${field}_set";
    $self->$member_func($newval);
}

sub this {
    my $ptr = shift;
    return tied(%$ptr);
}


# ------- FUNCTION WRAPPERS --------

package Prelude;

*checkVersion = *Preludec::checkVersion;

############# Class : Prelude::ClientProfile ##############

package Prelude::ClientProfile;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( Prelude );
%OWNER = ();
%ITERATORS = ();
sub new {
    my $pkg = shift;
    my $self = Preludec::new_ClientProfile(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        Preludec::delete_ClientProfile($self);
        delete $OWNER{$self};
    }
}

*getUid = *Preludec::ClientProfile_getUid;
*getGid = *Preludec::ClientProfile_getGid;
*getName = *Preludec::ClientProfile_getName;
*setName = *Preludec::ClientProfile_setName;
*getAnalyzerId = *Preludec::ClientProfile_getAnalyzerId;
*setAnalyzerId = *Preludec::ClientProfile_setAnalyzerId;
*getConfigFilename = *Preludec::ClientProfile_getConfigFilename;
*getAnalyzeridFilename = *Preludec::ClientProfile_getAnalyzeridFilename;
*getTlsKeyFilename = *Preludec::ClientProfile_getTlsKeyFilename;
*getTlsServerCaCertFilename = *Preludec::ClientProfile_getTlsServerCaCertFilename;
*getTlsServerKeyCertFilename = *Preludec::ClientProfile_getTlsServerKeyCertFilename;
*getTlsServerCrlFilename = *Preludec::ClientProfile_getTlsServerCrlFilename;
*getTlsClientKeyCertFilename = *Preludec::ClientProfile_getTlsClientKeyCertFilename;
*getTlsClientTrustedCertFilename = *Preludec::ClientProfile_getTlsClientTrustedCertFilename;
*getBackupDirname = *Preludec::ClientProfile_getBackupDirname;
*getProfileDirname = *Preludec::ClientProfile_getProfileDirname;
*setPrefix = *Preludec::ClientProfile_setPrefix;
*getPrefix = *Preludec::ClientProfile_getPrefix;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : Prelude::Connection ##############

package Prelude::Connection;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( Prelude );
%OWNER = ();
%ITERATORS = ();
sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        Preludec::delete_Connection($self);
        delete $OWNER{$self};
    }
}

sub new {
    my $pkg = shift;
    my $self = Preludec::new_Connection(@_);
    bless $self, $pkg if defined($self);
}

*getConnection = *Preludec::Connection_getConnection;
*close = *Preludec::Connection_close;
*connect = *Preludec::Connection_connect;
*setState = *Preludec::Connection_setState;
*getState = *Preludec::Connection_getState;
*setData = *Preludec::Connection_setData;
*getData = *Preludec::Connection_getData;
*getPermission = *Preludec::Connection_getPermission;
*setPeerAnalyzerid = *Preludec::Connection_setPeerAnalyzerid;
*getPeerAnalyzerid = *Preludec::Connection_getPeerAnalyzerid;
*getLocalAddr = *Preludec::Connection_getLocalAddr;
*getLocalPort = *Preludec::Connection_getLocalPort;
*getPeerAddr = *Preludec::Connection_getPeerAddr;
*getPeerPort = *Preludec::Connection_getPeerPort;
*isAlive = *Preludec::Connection_isAlive;
*getFd = *Preludec::Connection_getFd;
*recvIDMEF = *Preludec::Connection_recvIDMEF;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : Prelude::ConnectionPool ##############

package Prelude::ConnectionPool;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( Prelude );
%OWNER = ();
%ITERATORS = ();
sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        Preludec::delete_ConnectionPool($self);
        delete $OWNER{$self};
    }
}

sub new {
    my $pkg = shift;
    my $self = Preludec::new_ConnectionPool(@_);
    bless $self, $pkg if defined($self);
}

*init = *Preludec::ConnectionPool_init;
*setConnectionString = *Preludec::ConnectionPool_setConnectionString;
*getConnectionString = *Preludec::ConnectionPool_getConnectionString;
*getConnectionList = *Preludec::ConnectionPool_getConnectionList;
*setFlags = *Preludec::ConnectionPool_setFlags;
*getFlags = *Preludec::ConnectionPool_getFlags;
*setData = *Preludec::ConnectionPool_setData;
*getData = *Preludec::ConnectionPool_getData;
*addConnection = *Preludec::ConnectionPool_addConnection;
*delConnection = *Preludec::ConnectionPool_delConnection;
*setConnectionAlive = *Preludec::ConnectionPool_setConnectionAlive;
*setConnectionDead = *Preludec::ConnectionPool_setConnectionDead;
*setRequiredPermission = *Preludec::ConnectionPool_setRequiredPermission;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : Prelude::Client ##############

package Prelude::Client;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( Prelude::ClientProfile Prelude );
%OWNER = ();
%ITERATORS = ();
*ASYNC_SEND = *Preludec::Client_ASYNC_SEND;
*FLAGS_ASYNC_SEND = *Preludec::Client_FLAGS_ASYNC_SEND;
*ASYNC_TIMER = *Preludec::Client_ASYNC_TIMER;
*FLAGS_ASYNC_TIMER = *Preludec::Client_FLAGS_ASYNC_TIMER;
*HEARTBEAT = *Preludec::Client_HEARTBEAT;
*FLAGS_HEARTBEAT = *Preludec::Client_FLAGS_HEARTBEAT;
*CONNECT = *Preludec::Client_CONNECT;
*FLAGS_CONNECT = *Preludec::Client_FLAGS_CONNECT;
*AUTOCONFIG = *Preludec::Client_AUTOCONFIG;
*FLAGS_AUTOCONFIG = *Preludec::Client_FLAGS_AUTOCONFIG;
*IDMEF_READ = *Preludec::Client_IDMEF_READ;
*PERMISSION_IDMEF_READ = *Preludec::Client_PERMISSION_IDMEF_READ;
*ADMIN_READ = *Preludec::Client_ADMIN_READ;
*PERMISSION_ADMIN_READ = *Preludec::Client_PERMISSION_ADMIN_READ;
*IDMEF_WRITE = *Preludec::Client_IDMEF_WRITE;
*PERMISSION_IDMEF_WRITE = *Preludec::Client_PERMISSION_IDMEF_WRITE;
*ADMIN_WRITE = *Preludec::Client_ADMIN_WRITE;
*PERMISSION_ADMIN_WRITE = *Preludec::Client_PERMISSION_ADMIN_WRITE;
sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        Preludec::delete_Client($self);
        delete $OWNER{$self};
    }
}

sub new {
    my $pkg = shift;
    my $self = Preludec::new_Client(@_);
    bless $self, $pkg if defined($self);
}

*start = *Preludec::Client_start;
*init = *Preludec::Client_init;
*getClient = *Preludec::Client_getClient;
*sendIDMEF = *Preludec::Client_sendIDMEF;
*recvIDMEF = *Preludec::Client_recvIDMEF;
*getFlags = *Preludec::Client_getFlags;
*setFlags = *Preludec::Client_setFlags;
*getRequiredPermission = *Preludec::Client_getRequiredPermission;
*setRequiredPermission = *Preludec::Client_setRequiredPermission;
*getConfigFilename = *Preludec::Client_getConfigFilename;
*setConfigFilename = *Preludec::Client_setConfigFilename;
*getConnectionPool = *Preludec::Client_getConnectionPool;
*setConnectionPool = *Preludec::Client_setConnectionPool;
*__lshift__ = *Preludec::Client___lshift__;
*__rshift__ = *Preludec::Client___rshift__;
*setRecvTimeout = *Preludec::Client_setRecvTimeout;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : Prelude::PreludeLog ##############

package Prelude::PreludeLog;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( Prelude );
%OWNER = ();
%ITERATORS = ();
*DEBUG = *Preludec::PreludeLog_DEBUG;
*INFO = *Preludec::PreludeLog_INFO;
*WARNING = *Preludec::PreludeLog_WARNING;
*ERROR = *Preludec::PreludeLog_ERROR;
*CRITICAL = *Preludec::PreludeLog_CRITICAL;
*QUIET = *Preludec::PreludeLog_QUIET;
*SYSLOG = *Preludec::PreludeLog_SYSLOG;
*setLevel = *Preludec::PreludeLog_setLevel;
*setDebugLevel = *Preludec::PreludeLog_setDebugLevel;
*setFlags = *Preludec::PreludeLog_setFlags;
*getFlags = *Preludec::PreludeLog_getFlags;
*setLogfile = *Preludec::PreludeLog_setLogfile;
*setCallback = *Preludec::PreludeLog_setCallback;
sub new {
    my $pkg = shift;
    my $self = Preludec::new_PreludeLog(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        Preludec::delete_PreludeLog($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : Prelude::PreludeError ##############

package Prelude::PreludeError;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( Prelude );
%OWNER = ();
%ITERATORS = ();
sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        Preludec::delete_PreludeError($self);
        delete $OWNER{$self};
    }
}

sub new {
    my $pkg = shift;
    my $self = Preludec::new_PreludeError(@_);
    bless $self, $pkg if defined($self);
}

*getCode = *Preludec::PreludeError_getCode;
*what = *Preludec::PreludeError_what;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : Prelude::ClientEasy ##############

package Prelude::ClientEasy;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( Prelude::Client Prelude );
%OWNER = ();
%ITERATORS = ();
sub new {
    my $pkg = shift;
    my $self = Preludec::new_ClientEasy(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        Preludec::delete_ClientEasy($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : Prelude::IDMEFCriterion ##############

package Prelude::IDMEFCriterion;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( Prelude );
%OWNER = ();
%ITERATORS = ();
*OPERATOR_NOT = *Preludec::IDMEFCriterion_OPERATOR_NOT;
*OPERATOR_NOCASE = *Preludec::IDMEFCriterion_OPERATOR_NOCASE;
*OPERATOR_EQUAL = *Preludec::IDMEFCriterion_OPERATOR_EQUAL;
*OPERATOR_EQUAL_NOCASE = *Preludec::IDMEFCriterion_OPERATOR_EQUAL_NOCASE;
*OPERATOR_NOT_EQUAL = *Preludec::IDMEFCriterion_OPERATOR_NOT_EQUAL;
*OPERATOR_NOT_EQUAL_NOCASE = *Preludec::IDMEFCriterion_OPERATOR_NOT_EQUAL_NOCASE;
*OPERATOR_LESSER = *Preludec::IDMEFCriterion_OPERATOR_LESSER;
*OPERATOR_LESSER_OR_EQUAL = *Preludec::IDMEFCriterion_OPERATOR_LESSER_OR_EQUAL;
*OPERATOR_GREATER = *Preludec::IDMEFCriterion_OPERATOR_GREATER;
*OPERATOR_GREATER_OR_EQUAL = *Preludec::IDMEFCriterion_OPERATOR_GREATER_OR_EQUAL;
*OPERATOR_SUBSTR = *Preludec::IDMEFCriterion_OPERATOR_SUBSTR;
*OPERATOR_SUBSTR_NOCASE = *Preludec::IDMEFCriterion_OPERATOR_SUBSTR_NOCASE;
*OPERATOR_NOT_SUBSTR = *Preludec::IDMEFCriterion_OPERATOR_NOT_SUBSTR;
*OPERATOR_NOT_SUBSTR_NOCASE = *Preludec::IDMEFCriterion_OPERATOR_NOT_SUBSTR_NOCASE;
*OPERATOR_REGEX = *Preludec::IDMEFCriterion_OPERATOR_REGEX;
*OPERATOR_REGEX_NOCASE = *Preludec::IDMEFCriterion_OPERATOR_REGEX_NOCASE;
*OPERATOR_NOT_REGEX = *Preludec::IDMEFCriterion_OPERATOR_NOT_REGEX;
*OPERATOR_NOT_REGEX_NOCASE = *Preludec::IDMEFCriterion_OPERATOR_NOT_REGEX_NOCASE;
*OPERATOR_NULL = *Preludec::IDMEFCriterion_OPERATOR_NULL;
*OPERATOR_NOT_NULL = *Preludec::IDMEFCriterion_OPERATOR_NOT_NULL;
sub new {
    my $pkg = shift;
    my $self = Preludec::new_IDMEFCriterion(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        Preludec::delete_IDMEFCriterion($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : Prelude::IDMEFCriteria ##############

package Prelude::IDMEFCriteria;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( Prelude );
%OWNER = ();
%ITERATORS = ();
*OPERATOR_NOT = *Preludec::IDMEFCriteria_OPERATOR_NOT;
*OPERATOR_AND = *Preludec::IDMEFCriteria_OPERATOR_AND;
*OPERATOR_OR = *Preludec::IDMEFCriteria_OPERATOR_OR;
sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        Preludec::delete_IDMEFCriteria($self);
        delete $OWNER{$self};
    }
}

sub new {
    my $pkg = shift;
    my $self = Preludec::new_IDMEFCriteria(@_);
    bless $self, $pkg if defined($self);
}

*match = *Preludec::IDMEFCriteria_match;
*clone = *Preludec::IDMEFCriteria_clone;
*andCriteria = *Preludec::IDMEFCriteria_andCriteria;
*orCriteria = *Preludec::IDMEFCriteria_orCriteria;
*toString = *Preludec::IDMEFCriteria_toString;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : Prelude::IDMEFValue ##############

package Prelude::IDMEFValue;
use overload
    "<=" => sub { $_[0]->__le__($_[1])},
    ">=" => sub { $_[0]->__ge__($_[1])},
    "<" => sub { $_[0]->__lt__($_[1])},
    "!=" => sub { $_[0]->__ne__($_[1])},
    "==" => sub { $_[0]->__eq__($_[1])},
    ">" => sub { $_[0]->__gt__($_[1])},
    "=" => sub { my $class = ref($_[0]); $class->new($_[0]) },
    "fallback" => 1;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( Prelude );
%OWNER = ();
%ITERATORS = ();
*TYPE_UNKNOWN = *Preludec::IDMEFValue_TYPE_UNKNOWN;
*TYPE_INT8 = *Preludec::IDMEFValue_TYPE_INT8;
*TYPE_UINT8 = *Preludec::IDMEFValue_TYPE_UINT8;
*TYPE_INT16 = *Preludec::IDMEFValue_TYPE_INT16;
*TYPE_UINT16 = *Preludec::IDMEFValue_TYPE_UINT16;
*TYPE_INT32 = *Preludec::IDMEFValue_TYPE_INT32;
*TYPE_UINT32 = *Preludec::IDMEFValue_TYPE_UINT32;
*TYPE_INT64 = *Preludec::IDMEFValue_TYPE_INT64;
*TYPE_UINT64 = *Preludec::IDMEFValue_TYPE_UINT64;
*TYPE_FLOAT = *Preludec::IDMEFValue_TYPE_FLOAT;
*TYPE_DOUBLE = *Preludec::IDMEFValue_TYPE_DOUBLE;
*TYPE_STRING = *Preludec::IDMEFValue_TYPE_STRING;
*TYPE_TIME = *Preludec::IDMEFValue_TYPE_TIME;
*TYPE_DATA = *Preludec::IDMEFValue_TYPE_DATA;
*TYPE_ENUM = *Preludec::IDMEFValue_TYPE_ENUM;
*TYPE_LIST = *Preludec::IDMEFValue_TYPE_LIST;
*TYPE_CLASS = *Preludec::IDMEFValue_TYPE_CLASS;
*getType = *Preludec::IDMEFValue_getType;
*isNull = *Preludec::IDMEFValue_isNull;
sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        Preludec::delete_IDMEFValue($self);
        delete $OWNER{$self};
    }
}

sub new {
    my $pkg = shift;
    my $self = Preludec::new_IDMEFValue(@_);
    bless $self, $pkg if defined($self);
}

*match = *Preludec::IDMEFValue_match;
*clone = *Preludec::IDMEFValue_clone;
*toString = *Preludec::IDMEFValue_toString;
*__le__ = *Preludec::IDMEFValue___le__;
*__ge__ = *Preludec::IDMEFValue___ge__;
*__lt__ = *Preludec::IDMEFValue___lt__;
*__gt__ = *Preludec::IDMEFValue___gt__;
*__eq__ = *Preludec::IDMEFValue___eq__;
*__ne__ = *Preludec::IDMEFValue___ne__;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : Prelude::IDMEFPath ##############

package Prelude::IDMEFPath;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( Prelude );
%OWNER = ();
%ITERATORS = ();
sub new {
    my $pkg = shift;
    my $self = Preludec::new_IDMEFPath(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        Preludec::delete_IDMEFPath($self);
        delete $OWNER{$self};
    }
}

*set = *Preludec::IDMEFPath_set;
*getClass = *Preludec::IDMEFPath_getClass;
*getValueType = *Preludec::IDMEFPath_getValueType;
*setIndex = *Preludec::IDMEFPath_setIndex;
*undefineIndex = *Preludec::IDMEFPath_undefineIndex;
*getIndex = *Preludec::IDMEFPath_getIndex;
*makeChild = *Preludec::IDMEFPath_makeChild;
*makeParent = *Preludec::IDMEFPath_makeParent;
*compare = *Preludec::IDMEFPath_compare;
*clone = *Preludec::IDMEFPath_clone;
*checkOperator = *Preludec::IDMEFPath_checkOperator;
*getApplicableOperators = *Preludec::IDMEFPath_getApplicableOperators;
*getName = *Preludec::IDMEFPath_getName;
*isAmbiguous = *Preludec::IDMEFPath_isAmbiguous;
*hasLists = *Preludec::IDMEFPath_hasLists;
*isList = *Preludec::IDMEFPath_isList;
*getDepth = *Preludec::IDMEFPath_getDepth;
*get = *Preludec::IDMEFPath_get;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : Prelude::IDMEFTime ##############

package Prelude::IDMEFTime;
use overload
    "<=" => sub { $_[0]->__le__($_[1])},
    "!=" => sub { $_[0]->__ne__($_[1])},
    ">=" => sub { $_[0]->__ge__($_[1])},
    "<" => sub { $_[0]->__lt__($_[1])},
    "==" => sub { $_[0]->__eq__($_[1])},
    ">" => sub { $_[0]->__gt__($_[1])},
    "=" => sub { my $class = ref($_[0]); $class->new($_[0]) },
    "fallback" => 1;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( Prelude );
%OWNER = ();
%ITERATORS = ();
sub new {
    my $pkg = shift;
    my $self = Preludec::new_IDMEFTime(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        Preludec::delete_IDMEFTime($self);
        delete $OWNER{$self};
    }
}

*set = *Preludec::IDMEFTime_set;
*setSec = *Preludec::IDMEFTime_setSec;
*setUSec = *Preludec::IDMEFTime_setUSec;
*setGmtOffset = *Preludec::IDMEFTime_setGmtOffset;
*getSec = *Preludec::IDMEFTime_getSec;
*getUSec = *Preludec::IDMEFTime_getUSec;
*getGmtOffset = *Preludec::IDMEFTime_getGmtOffset;
*getTime = *Preludec::IDMEFTime_getTime;
*clone = *Preludec::IDMEFTime_clone;
*toString = *Preludec::IDMEFTime_toString;
*__ne__ = *Preludec::IDMEFTime___ne__;
*__ge__ = *Preludec::IDMEFTime___ge__;
*__le__ = *Preludec::IDMEFTime___le__;
*__eq__ = *Preludec::IDMEFTime___eq__;
*__gt__ = *Preludec::IDMEFTime___gt__;
*__lt__ = *Preludec::IDMEFTime___lt__;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : Prelude::IDMEFClass ##############

package Prelude::IDMEFClass;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( Prelude );
%OWNER = ();
%ITERATORS = ();
sub new {
    my $pkg = shift;
    my $self = Preludec::new_IDMEFClass(@_);
    bless $self, $pkg if defined($self);
}

*getDepth = *Preludec::IDMEFClass_getDepth;
*get = *Preludec::IDMEFClass_get;
*getChildCount = *Preludec::IDMEFClass_getChildCount;
*isList = *Preludec::IDMEFClass_isList;
*isKeyedList = *Preludec::IDMEFClass_isKeyedList;
*getName = *Preludec::IDMEFClass_getName;
*toString = *Preludec::IDMEFClass_toString;
*getValueType = *Preludec::IDMEFClass_getValueType;
*getAttributes = *Preludec::IDMEFClass_getAttributes;
*getPath = *Preludec::IDMEFClass_getPath;
*getEnumValues = *Preludec::IDMEFClass_getEnumValues;
*getApplicableOperator = *Preludec::IDMEFClass_getApplicableOperator;
sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        Preludec::delete_IDMEFClass($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : Prelude::IDMEF ##############

package Prelude::IDMEF;
use overload
    "==" => sub { $_[0]->__eq__($_[1])},
    "=" => sub { my $class = ref($_[0]); $class->new($_[0]) },
    "fallback" => 1;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( Prelude );
%OWNER = ();
%ITERATORS = ();
sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        Preludec::delete_IDMEF($self);
        delete $OWNER{$self};
    }
}

sub new {
    my $pkg = shift;
    my $self = Preludec::new_IDMEF(@_);
    bless $self, $pkg if defined($self);
}

*set = *Preludec::IDMEF_set;
*clone = *Preludec::IDMEF_clone;
*getId = *Preludec::IDMEF_getId;
*toString = *Preludec::IDMEF_toString;
*toJSON = *Preludec::IDMEF_toJSON;
*__eq__ = *Preludec::IDMEF___eq__;
*write = *Preludec::IDMEF_write;
*read = *Preludec::IDMEF_read;
*get = *Preludec::IDMEF_get;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


# ------- VARIABLE STUBS --------

package Prelude;

1;
