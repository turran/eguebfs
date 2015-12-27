#include <Eguebfs.h>
#include <Ecore.h>
#include <Efl_Egueb.h>

int main(int arcg, char **argv)
{
	Eguebfs *efs;
	Egueb_Dom_Node *doc;
	Enesim_Stream *stream;
	Eina_Bool ret;

	ecore_init();
	efl_egueb_init();

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
	/* mount and wait */
	efs = eguebfs_mount(doc, argv[2]);
	ecore_main_loop_begin();
	eguebfs_umount(efs);
shutdown:
	efl_egueb_shutdown();
	ecore_shutdown();
	return 0;
}
