# Network Common Data Format for C++1y

Or NetCDF for short, is not exactly new. Neither are API that know how to handle the file format. I started out
this repository with the express design goal that the core model should inherently, intrinsically know how to
handle itself, while at the same time leveraging present day C++1y language features. This repository provides
the core API and ability to load and save an NC classic file.

Assumptions that I make include that compilers and linkers will perform adequate optimization that make doing
so highly productive.

## Disclaimers

This is not a finished work by any stretch of the imagination. There still remains some work to make this a
useful, production ready library. The authors are not responsible for any errors you may experience directly
or indirectly. See licensing for clarification.

## Features

As mentioned, I set out with the design goal that CDF behavior should be inherent, intrinsic in the core model.
For example, when a Dimension is added, it simply gets added to a
[std::vector](http://www.cplusplus.com/reference/vector/vector/). Same goes for Attributes and Variables.
Similarly, corresponding Values and Data are an inherent part of both Attributes and Variables.

Heavy use of vectors and iterators is used as makes sense to do so. Additionally, C++1y features such as
functional injections are done to help the logic flow seemlessly.

I have also used template functions in order to better leverage common areas of the core model.

A reader/writer pair have been provided for convenience reading and writing binary formatted NC files. Some
examples have been included for purposes of illustration only, which are also readily available via
[UniData](http://www.unidata.ucar.edu/software/netcdf/examples/files.html).

## Feature Highlights

API is constructed along functional slices. Underlying property types are provided, such as strongly typed
enumerations closely aligned with the file format shape itself (i.e. nc_type). Top level components are as one
might expect, in alphabetical order:

* [attr](http://github.com/mwpowellhtx/netcdfcpp1y/blob/master/src/netcdf/parts/attr.h): Attribute
* [dim](http://github.com/mwpowellhtx/netcdfcpp1y/blob/master/src/netcdf/parts/dim.h): Dimension
* [netcdf](http://github.com/mwpowellhtx/netcdfcpp1y/blob/master/src/netcdf/netcdf.h): The top level model
* [var](http://github.com/mwpowellhtx/netcdfcpp1y/blob/master/src/netcdf/parts/var.h): Variable

Details such as the entity type, number of elements, whether present or absent, are all intrinsically expressed
as part of the model. These are really only important at the moment of load and/or save.

Second-tier components include, in alphabetical order:

* [magic](http://github.com/mwpowellhtx/netcdfcpp1y/blob/master/src/netcdf/parts/magic.h): Captures the &lsquo;magic&rsquo; fields in the file format
* [value](http://github.com/mwpowellhtx/netcdfcpp1y/blob/master/src/netcdf/parts/value.h): Value, used for both Attribute values as well as Variable values

Plus corresponding [vectors](http://www.cplusplus.com/reference/vector/vector/) as makes sense to do so.

Intrinsic slices include, in alphabetical order:

* [attributable](http://github.com/mwpowellhtx/netcdfcpp1y/blob/master/src/netcdf/parts/attributable.h): Variables and the NetCDF itself have Attributes associated with them
* [named](http://github.com/mwpowellhtx/netcdfcpp1y/blob/master/src/netcdf/parts/named.h): Dimension, Attribute, and Variable each have a Name associated with them
* [valuable](http://github.com/mwpowellhtx/netcdfcpp1y/blob/master/src/netcdf/parts/valuable.h): Attributes and Variables may both have Values associated with them

Vectored access is done using either an index, offset from begining of respective vector, or name.
When appropriate a corresponding vector iterator will be returned.

API dealing with values does so in as transparent a manner as possible using template functions. Generally and
where applicable, developers can specify a name and a vector of arbitrarily typed, though supported, values, and
the template functions will determine the most appropriate shape for the file format data intrinsically.

For example, consider the following illustration. For clarity, I will specify the template types:

```C++
attributable & aVar = var();
aVar.add_attr<std::vector<int32_t>>("some_ints", { 1, 2, 3 });
```

This will yield an Attributable Variable with one Attribute named &quot;some_ints&quot;, and corresponding
values. The nc_type will be determined to be nc_int based on the underlying
[typeid](http://en.cppreference.com/w/cpp/language/typeid) of the
[vector](http://www.cplusplus.com/reference/vector/vector/) value_type, in this case,
``std::vector<int32_t>::value_type``.

String text values are presently a  use case with their own specialized API. An attempt was made to treat text
[std::string](http://www.cplusplus.com/reference/string/string/) as a vector of characters (literally, nc_char),
but the use case is just too specialized to make this feasible for both cases. Potentially, language features
such as [std::enable_if](http://en.cppreference.com/w/cpp/types/enable_if) could be put to use here, but that
would be a future work.

It is important to note that the model treats text values as a value vector containing a single value instance,
whose text member variable contains the string of interest. This is instead of having value instances for each
character, for efficiency purposes. Otherwise, primitively typed values are individually stored in a values vector.
Consistent with the model design goals, details like number of elements is an intrinsic part of the model, which is
simply determined using the [std::vector::size](http://www.cplusplus.com/reference/vector/vector/size/) function.

## Bucket List

The following items are areas I would like to better address and/or which require attention in order to round the
library out and/or finish it for production use. This is not exhaustive, and require elaboration, at the time of
this writing:

* Provide additional readers/writers: i.e.
[CDL](http://www.unidata.ucar.edu/software/netcdf/workshops/2011/utilities/CDL.html),
[JSON](http://json.org/), or even [Xml](http://www.w3.org/TR/xml11/)
* Improve functionality along the lines of Dimensionality, Attributing, and Variables
* Provide API for access along potentially arbitrary number of Dimensions
* Improve transparancy working with string text values as compared with primitively typed values
* Provide netCDF-4 support, for things like
[grouping](http://www.unidata.ucar.edu/software/netcdf/workshops/2010/groups-types/Introduction.html)
first-class [types](http://www.unidata.ucar.edu/software/netcdf/workshops/2010/groups-types/Introduction.html)
* Extend support for interested subject areas; beyond the scope of this repository, per se, except as referenced
as a submodule
