#include "yfrogUploadStatus.h"

YfrogUploadStatus::YfrogUploadStatus(const QDomDocument& xml)
{
	setStatus(UnknownError);

	QDomNodeList response = xml.elementsByTagName("rsp");
	if (response.count() > 0)
	{
		QDomNode node = response.at(0);
		QDomNodeList children = node.childNodes();
		QDomNode status = node.attributes().namedItem("status");
		if (status.isNull())
		{
			status = node.attributes().namedItem("stat");
		}

		if (!status.nodeValue().compare("ok"))
		{
			setStatus(Ok);
			for (int i = 0; i < children.count(); i++)
			{
				QDomNode node = children.at(i);
				if (!node.nodeName().compare("mediaid", Qt::CaseInsensitive))
				{
					setMediaId(node.toElement().text());
				}
				else if (!node.nodeName().compare("mediaurl", Qt::CaseInsensitive))
				{
					setMediaUrl(node.toElement().text());
				}
			}
		}
		else
		{
			if (children.count() > 0)
			{
				QDomNode err = children.at(0);
				if (!err.nodeName().compare("err"))
				{
					QDomNamedNodeMap attr = err.attributes();
					QDomNode code = attr.namedItem("code");
					if (!code.nodeValue().compare("1001"))
					{
						setStatus(AuthFailed);
					}
					else if (!code.nodeValue().compare("1002"))
					{
						setStatus(MediaNotFound);
					}
					else if (!code.nodeValue().compare("1003"))
					{
						setStatus(UnsupportedMediaType);
					}
					else if (!code.nodeValue().compare("1004"))
					{
						setStatus(MediaTooBig);
					}
				}
			}
		}
	}
}

