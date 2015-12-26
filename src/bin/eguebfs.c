#include <Eguebfs.h>

int main(int arcg, char **argv)
{
	Egueb_Dom_Node *doc;
	Enesim_Stream *stream;
	Eina_Bool ret;

	egueb_dom_init();
	stream = enesim_stream_file_new(argv[1], "r");
	if (!stream)
	{
		printf("Fail to load file %s\n", argv[1]);
		goto shutdown;
	}
	/* create the document */
	ret = egueb_dom_parser_parse(stream, &doc);
	if (!ret)
	{
		printf("Fail to parse file %s\n", argv[1]);
		goto shutdown;
	}
	eguebfs_mount(doc, argv[2]);
shutdown:

	egueb_dom_shutdown();
	return 0;
}
