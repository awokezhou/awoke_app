
import os
import time
import copy
import xml.sax



class QDBObjectMember(object):

    def __init__(self, name, dtype, power, default, maxlength, dmin, dmax, description):
        self.name = name
        self.dtype = dtype
        self.power = power
        self.default = default
        self.maxlength = maxlength
        self.min = dmin
        self.max = dmax
        self.description = description


class QDBObject(object):

    def __init__(self, oid, name, category, description):
        self.oid = oid
        self.category = category
        self.description = description
        self.members = []
        self.childs = []
        self.parent = None
        self.parent_name = ''
        self.fullname = name
        self.name = ''
        self.parse_name(name)
        self.oidname = self.getoidname(self.name)

    def seqTravel(self):
        yield self
        for c in self.childs:
            for _c in c.seqTravel():
                yield _c

    def revTravel(self):
        for c in self.childs:
            for _c in c.revTravel():
                yield _c
        yield self

    def childadd(self, child):
        child.parent = self
        self.childs.append(child)

    def parse_name(self, name):
        if name.find('.'):
            parts = name.split('.')
            if (parts[-1] == '') or (parts[-1] == '{i}'):
                self.name = parts[-2]
            else:
                self.name = parts[-1]
                self.parent_name = parts[-2]
        else:
            self.name = name
        print('name:{} selfname:{} parent:{}'.format(name, self.name, self.parent))

    def getoidname(self, name):
        start = 0
        end = 0
        part = []
        index = []
        macros = 'QDB_OID_'
        for c in name:
            if c.isupper():
                idx = name.index(c)
                if idx > 0:
                    index.append(name.index(c))

        for i in index:
            end = i
            part.append(name[start:end].upper())
            start = end
        part.append(name[start:].upper())
        
        for p in part:
            macros += '{}_'.format(p)
        return macros[:-1]


class XmlHandler(xml.sax.ContentHandler):
    
    DEFAULT_CONFIG = {
        'version':'',
        'rootnode':'',
        'objectnode':'',
        'membernode':'',
        'support_category':'',
        'support_power':'',
        'support_dtype':'',
        'dtype_default':'',
    }

    def __init__(self, **configs):
        self.config = copy.copy(self.DEFAULT_CONFIG)
        for key in self.config:
            if key in configs:
                self.config[key] = configs.pop(key)
        self.objects = []
        self.object_count = 0
        self.member_count = 0
        self.rootnode = None
        self.currobj = None


    def rootnode_parse(self, tag, attributes):
        if tag == self.config['rootnode']:
            self.rootnode = tag
            print('version:{} {}'.format(attributes['version'], self.config['version']))
            if attributes['version'] == self.config['version']:
                self.version = attributes['version']
                print('rootnode:{} version:{}'.format(
                    self.rootnode, self.version
                ))
            else:
                raise Exception('version not support, parser version:{} but metadata version:{}'.format(
                    self.config['version'], attributes['version']))
        else:
            raise Exception('rootnode not {}'.format(self.config['rootnode']))


    def find_parent(self, name):
        for obj in self.objects:
            if obj.name == name:
                return obj
            for c in obj.seqTravel():
                if c.name == name:
                    return c

    def object_parse(self, attributes):
        if not attributes.get('name'):
            raise Exception('Object[{}] has no name'.format(self.object_count))
        else:
            name = attributes['name']

        if not attributes.get('category'):
            raise Exception('Object[{}] has no category'.format(self.object_count))
        else:
            category = attributes['category']
            if category not in self.config['support_category']:
                raise Exception('Object:{} category not support'.format(name))

        if not attributes.get('description'):
            print('Object[{}] has no description, use \'\' instead'.format(name))
            description = ''
        else:
            description = attributes['description']

        self.currobj = QDBObject(self.object_count, name, category, description)
        if self.currobj.parent_name == '':
            self.objects.append(self.currobj)
        else:
            parent = self.find_parent(self.currobj.parent_name)
            print('find parent:{}'.format(parent.name))
            parent.childadd(self.currobj)

    def member_name_parse(self, attributes):
        obj = self.currobj
        if not attributes.get('name'):
            raise Exception('Object:{}.member[{}] has no name'.format(obj.name, len(obj.members)+1))
        else:
            return attributes['name']

    def member_dtype_parse(self, attributes):
        obj = self.currobj
        dtype = None
        if not attributes.get('dtype'):
            raise Exception('Object:{}.member[{}] has no dtype'.format(obj.name, len(obj.members)+1))
        else:
            dtype = attributes['dtype']
        
        if dtype not in self.config['support_dtype']:
            raise Exception('Object:{}.member[{}] dtype not support'.format(obj.name, len(obj.members)+1))

        return dtype

    def member_power_parse(self, attributes):
        obj = self.currobj
        power = None
        if not attributes.get('power'):
            raise Exception('Object:{}.member[{}] has no power'.format(obj.name, len(obj.members)+1))
        else:
            power = attributes['power']
        
        if power not in self.config['support_power']:
            raise Exception('Object[{}].member[{}] power not support'.format(obj.name, len(obj.members)+1))

        return power

    def member_default_parse(self, attributes, dtype):
        default = None
        if attributes.get('default'):
            default = attributes['default']
        else:
            index = self.config['support_dtype'].index(dtype)
            default = self.config['dtype_default'][index]
        
        return default

    def member_maxlen_parse(self, attributes, dtype):
        maxlen = 0
        if dtype == 'String':
            if attributes.get('maxlength'):
                maxlen = attributes['maxlength']
            else:
                maxlen = 0
        return maxlen

    def member_min_parse(self, attributes, dtype):
        dmin = 0
        needmin_dtype = ['Int','UnsignedInt','Long','UnsignedLong']
        if dtype in needmin_dtype:
            if attributes.get('min'):
                dmin = attributes['min']
            else:
                dmin = 0
        return dmin

    def member_max_parse(self, attributes, dtype):
        dmax = 0
        needmax_dtype = ['Int','UnsignedInt','Long','UnsignedLong']
        if dtype in needmax_dtype:
            if attributes.get('min'):
                dmax = attributes['min']
            else:
                dmax = 0
        return dmax

    def object_member_parse(self, attributes):
        name = self.member_name_parse(attributes)
        dtype = self.member_dtype_parse(attributes)
        power = self.member_power_parse(attributes)
        default = self.member_default_parse(attributes, dtype)
        maxlength = self.member_maxlen_parse(attributes, dtype)
        dmin = self.member_min_parse(attributes, dtype)
        dmax = self.member_max_parse(attributes, dtype)

        if not attributes.get('description'):
            print('Object[{}].member[{}] has no description, use \'\' instead'.format(self.currobj.oid, 
                len(self.currobj.members)+1))
            description = ''
        else:
            description = attributes['description']

        member = QDBObjectMember(name, dtype, power, default, maxlength, dmin, dmax, description)
        self.currobj.members.append(member)


    def startElement(self, tag, attributes):
        if not self.rootnode:
            self.rootnode_parse(tag, attributes)
        elif tag == self.config['objectnode']:
            self.object_count += 1
            self.object_parse(attributes)
        elif tag == self.config['membernode']:
            self.object_member_parse(attributes)
        else:
            raise Exception('Unknown tag:{}'.format(tag))

class QDBParser(object):
    
    DEFAULT_CONFIG = {
        'version':'1.0',
        'metadata':'database.xml',
        'rootnode':'QuickDatabase',
        'objectnode':'object',
        'membernode':'member',
        'support_category':['Static', 'Multiple', 'Dynamic'],
        'support_power':['ReadOnly', 'WriteOnly', 'ReadWrite'],
        'support_dtype':['String','Int','UnsignedInt','Long','UnsignedLong','Boolean','DateTime','Base64','Hex'],
        'dtype_default':['\"\"','0','0','0','0','true','\"\"', '0','0x0'],
        'dtype2C':{'String':'char *','Int':'int32_t','UnsignedInt':'uint32_t','Long':'int64_t','UnsignedLong':'uint64_t',
                     'Boolean':'bool','DateTime':'char *', 'Base64':'uint8_t','Hex':'uint8_t'},
        'dtype2CMT':{'String':'QDB_MT_STRING','Int':'QDB_MT_INTEGER','UnsignedInt':'QDB_MT_UNSIGNED_INTEGER',
                       'Long':'QDB_MT_LONG','UnsignedLong':'QDB_MT_UNSIGNED_LONG','Boolean':'QDB_MT_BOOLEAN',
                       'DateTime':'QDB_MT_DATETIME','Base64':'QDB_MT_BASE64','Hex':'QDB_MT_HEX'},
        'dtype2CMF':{'ReadOnly':'QDB_MF_RO','WriteOnly':'QDB_MF_WO','ReadWrite':'QDB_MF_WR'}
    }

    def __init__(self, **configs):
        
        self.config = copy.copy(self.DEFAULT_CONFIG)
        for key in self.config:
            if key in configs:
                self.config[key] = configs.pop(key)

        self.objects = []

        self.parser = xml.sax.make_parser()
        self.parser.setFeature(xml.sax.handler.feature_namespaces, 0)
        self.handler = XmlHandler(rootnode=self.config['rootnode'],
                                  version=self.config['version'],
                                  objectnode=self.config['objectnode'],
                                  membernode=self.config['membernode'],
                                  support_category=self.config['support_category'],
                                  support_power=self.config['support_power'],
                                  support_dtype=self.config['support_dtype'],
                                  dtype_default=self.config['dtype_default'])
        self.parser.setContentHandler(self.handler)

    def parse(self):
        self.parser.parse(self.config['metadata'])
        self.objects = self.handler.objects
        self.object_nr = self.handler.object_count

    def object_info(self, obj):
        print('\nobject:{} {}'.format(obj.name, obj.category))
        for m in obj.members:
            print('\tname:{} dtype:{} power:{} default:{} maxlen:{} min:{} max:{}'.format(
                m.name,
                m.dtype,
                m.power,
                m.default,
                m.maxlength,
                m.min,
                m.max
            ))

    def info(self):
        print('object number:{}'.format(len(self.objects)))
        for obj in self.objects:
            for c in obj.seqTravel():
                self.object_info(c)

    def generate_object_struct(self, filename):
        
        fobj = open(filename, 'w+')
        string = '#ifndef __QDB_OBJECT_H__\n'
        string += '#define __QDB_OBJECT_H__\n\n\n'
        string += '#include \"QDB.h\"\n\n\n'
        string += '#define __QDB_VERSION__\t\t"{}"\n\n'.format(self.handler.version)
        fobj.write(string)
        for obj in self.objects:
            for c in obj.seqTravel():
                print('object name:{}'.format(c.name))
                string = ''
                string += '\n\n\n/* -- Object define -- {*/\n'
                string += '/*\n'
                string += ' * oid: {} {}\n'.format(c.oidname, c.oid)
                string += ' * name: {}\n'.format(c.name)
                string += ' * category: {}\n'.format(c.category)
                string += ' * description: {}\n'.format(c.description)
                string += ' */\n'
                string += 'typedef struct _QDBObject{}'.format(c.name) + ' {\n'
                for m in c.members:
                    string += '\t{} {};'.format(self.config['dtype2C'][m.dtype], m.name)
                    if m.description:
                        string += '\t\t/* {} */'.format(m.description)
                    string += '\n'
                string += '} ' + 'QDBObject{};\n'.format(c.name)
                string += '\n/*} -- ' + 'Object define -- */\n'
                fobj.write(string)
        string = '\n\n\n#endif /* __QDB_OBJECT_H__ */'
        fobj.write(string)
        fobj.close()

    def generate_objectid(self, filename):
        fobj = open(filename, 'w+')
        string = '#ifndef __QDB_OBJECTID_H__\n'
        string += '#define __QDB_OBJECTID_H__\n\n\n'
        string += '#include \"QDB.h\"\n\n\n'
        string += '#define QDB_OID_MAXSIZE\t\t{}\n\n'.format(self.object_nr)
        fobj.write(string)
        for obj in self.objects:
            for c in obj.seqTravel():
                string = '/* object name: {} */\n'.format(c.name)
                string += '#define {}\t{}\n\n'.format(c.oidname, c.oid)
                fobj.write(string)
        string = '#define QDB_OID_MAX\t{}\n'.format(self.object_nr+1)
        string += '\n\n\n#endif /* __QDB_OBJECTID_H__ */'
        fobj.write(string)
        fobj.close()

    def member_dump(self, objname, members):
        string = 'QDBObjectMember {}ObjectMembers[] = '.format(objname) + '{\n'
        for m in members:
            string += '\t{\n'
            string += '\t\t\"{}\",\n'.format(m.name)
            string += '\t\t{},\n'.format(self.config['dtype2CMT'][m.dtype])
            string += '\t\t{},\n'.format(self.config['dtype2CMF'][m.power])
            string += '\t\t0,\n'
            string += '\t\t{},\n'.format(str(m.default))
            string += '\t\t{(void *)0, (void *)0}  /* validator data */\n'
            string += '\t},\n'
        string += '};\n\n'
        return string

    def object_dump(self, obj):
        string = 'QDBObject {}Object = '.format(obj.name) + '{\n'
        string += '\t{}, /* oid:{} */\n'.format(obj.oid, obj.oidname)
        string += '\t\"{}\", /* object name */\n'.format(obj.name)
        string += '\t0,0, /* flags and instance depth */\n'
        string += '\tNULL, /* parent */\n'
        #string += '\t{0, QDB_ACCESS_BITMASK, 0, QDB_NOTIFICATION_VALUE, 0, 0}, /* nodeattr value of this objNode */\n'
        #string += '\tNULL, /* pointer to instances of nodeattr values for parameter nodes */\n'
        if (obj.category == 'Static') and (len(obj.members) > 0):
            string += '\tsizeof({}ObjectMembers)/sizeof(QDBObjectMember), /* members number */\n'.format(obj.name)
            string += '\t{}ObjectMembers, /* point to members */\n'.format(obj.name)
        else:
            string += '\t0, /* members number */\n'
            string += '\tNULL, /* point to members */\n'
        if len(obj.childs) > 0:
            string += '\tsizeof({}Childs)/sizeof(QDBObject), /* childs number */\n'.format(obj.name)
            string += '\t{}Childs, /* point to childs */\n'.format(obj.name)
        else:
            string += '\t0, /* members number */\n'
            string += '\tNULL, /* point to members */\n'
        string += '};\n\n'
        return string

    def object_childs_dump(self, obj):
        string = 'QDBObject {}Childs[] = '.format(obj.name) + '{\n'
        for child in obj.childs:
            string += '\t{\n'
            string += '\t\t{},\n'.format(child.oid)
            string += '\t\t\"{}\", /* name:{} */\n'.format(child.name, child.fullname)
            string += '\t\t0,0, /* flags and instance depth */\n'
            string += '\t\tNULL, /* parent */\n'
            #string += '\t\t{0, DEFAULT_ACCESS_BIT_MASK, 0, DEFAULT_NOTIFICATION_VALUE, 0, 0}, /* nodeattr value of this objNode */\n'
            #string += '\t\tNULL, /* pointer to instances of nodeattr values for parameter nodes */\n'
            if (child.category == 'Static') and (len(child.members) > 0):
                string += '\t\tsizeof({}ObjectMembers)/sizeof(QDBObjectMember), /* members number */\n'.format(child.name)
                string += '\t\t{}ObjectMembers, /* point to members */\n'.format(child.name)
            else:
                string += '\t\t0, /* members number */\n'
                string += '\t\tNULL, /* point to members */\n'
            if len(child.childs) > 0:
                string += '\t\tsizeof({}Childs)/sizeof(QDBObject), /* childs number */\n'.format(child.name)
                string += '\t\t{}Childs, /* point to childs */\n'.format(child.name)
            else:
                string += '\t\t0, /* members number */\n'
                string += '\t\tNULL, /* point to members */\n'
            string += '\t},\n'
        string += '};\n\n'
        return string

    def generate_object(self, obj, fobj):
        print('object:{}'.format(obj.name))
        if len(obj.members) > 0:
            string = self.member_dump(obj.name, obj.members)
            fobj.write(string)
        if len(obj.childs) > 0:
            string = self.object_childs_dump(obj)
            fobj.write(string)
        string = self.object_dump(obj)
        fobj.write(string)

    def generate_source(self, filename):
        fobj = open(filename, 'w+')
        string = '#include "QDB.h"\n\n\n'
        fobj.write(string)
        for obj in self.objects:
            for c in obj.revTravel():
                self.generate_object(c, fobj)
        fobj.close()

    def generate(self, prefix='QDB'):
        
        # generate include file
        incfname = '{}_object.h'.format(prefix)
        self.generate_object_struct(incfname)
        print('generate {}'.format(incfname))

        # generate objectid file
        oidfname = '{}_objectid.h'.format(prefix)
        self.generate_objectid(oidfname)
        print('generate {}'.format(oidfname))

        # generate source file
        srcfname = '{}_object.c'.format(prefix)
        self.generate_source(srcfname)
        print('generate {}'.format(srcfname))


class ct(object):
    def __init__(self, name):
        self.name = name
        self.parent = None
        self.childs = []
    def childadd(self, child):
        child.parent = self
        self.childs.append(child)
    def seqTravel(self):
        yield self
        for c in self.childs:
            for _c in c.seqTravel():
                yield _c

    def revTravel(self):
        for c in self.childs:
            for _c in c.revTravel():
                yield _c
        yield self

def childtest():
    A = ct('A')
    B = ct('B')
    C = ct('C')
    D = ct('D')
    E = ct('E')
    F = ct('F')
    G = ct('G')
    H = ct('H')
    I = ct('I')
    J = ct('J')

    A.childadd(B)
    A.childadd(C)
    A.childadd(D)
    D.childadd(E)
    D.childadd(F)
    D.childadd(G)
    D.childadd(H)
    H.childadd(I)
    H.childadd(J)
    print('-- seqTravel --')
    for c in A.seqTravel():
        print('{}'.format(c.name))

    print('-- revTravel --')
    for c in A.revTravel():
        print('{}'.format(c.name))

if __name__ == "__main__":
    
    
    parser = QDBParser(metadata='database.xml')
    try:
        parser.parse()
        parser.info()
        parser.generate()
    except Exception as e:
        print(e)
    """
    childtest()
    """