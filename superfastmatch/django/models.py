"""
This set of models is a Django interface to the SuperFastMatch algorithm and the Kyoto Cabinet/Tycoon document index.

The following `settings <http://docs.djangoproject.com/en/dev/ref/settings>`_ can be used to configure how to communicate with Kyoto Tycoon::
    
    SUPERFASTMATCH_HOST = '127.0.0.1'
    SUPERFASTMATCH_PORT = 1977
    
as well as a minimum percentage of the document required to be copied for an association to be created:

    SUPERFASTMATCH_MIN_THRESHOLD = 0.02

and you must remember to include both :mod:`superfastmatch.django` and 
`django.contrib.contenttypes <http://docs.djangoproject.com/en/dev/ref/contrib/contenttypes/>`_ 
in `INSTALLED_APPS <http://docs.djangoproject.com/en/dev/ref/settings/#installed-apps>`_. For example::

    INSTALLED_APPS=(
                        'superfastmatch.django',
                        'django.contrib.contenttypes'
                    )

Example model definition:

>>> import re
>>> from superfastmatch.django.models import Document
>>> class NewsArticle(Document):
...   @property
...   def clean(self):
...     return re.sub('<[^<]+?>', '', self.content)

Example usage:

>>> article_one = NewsArticle(content="An example news article")
>>> article_one.save()
>>> article_two = NewsArticle(content="Another example news article with a bit more content")
>>> article_two.save()
>>> NewsArticle.objects.search('example news article')
defaultdict(<type 'dict'>, {<class 'superfastmatch.django.tests.NewsArticle'>: OrderedDict([(<NewsArticle: NewsArticle object>, 21), (<NewsArticle: NewsArticle object>, 21)])})
>>> article_one.delete()
>>> article_two.delete()
"""

from django.db import models
from django.contrib.contenttypes.models import ContentType
from django.contrib.contenttypes import generic
from django.core.exceptions import ObjectDoesNotExist
from django.conf import settings
from collections import defaultdict
from superfastmatch.ordereddict import OrderedDict
from superfastmatch.kyototycoon import KyotoTycoon

def get_tycoon():
    return KyotoTycoon(host=getattr(settings,'SUPERFASTMATCH_HOST','127.0.0.1'),port=getattr(settings,'SUPERFASTMATCH_PORT',1978))

class DocumentManager(models.Manager):
    """Manager for running bulk operations on Documents"""

    def associate(self):
        """Method for updating associations between all documents"""
        for content in Content.objects.all():
            content.similar.all().delete()
            min_threshold = int(round(getattr(settings,'SUPERFASTMATCH_MIN_THRESHOLD',0.02)*len(content.content)))
            for result in self.search(content.content,min_threshold=min_threshold).values():
                for document,score in result.items():
                    to_content=document.cleaned_content.get()
                    if content!=to_content:
                        association=Association(from_content=content,
                                                to_content=to_content,
                                                common_characters=score,
                                                common_percentage=float(score)/len(content.content)
                                            )
                        association.save()

    def search(self,text,document_types=[],min_threshold=1):
        """
        Method for searching for text in all or specified documents
        
        :param text: The text to search for.
        :param document_types: A list or tuple of the document subclasses to include in the results. If not specified, all document types are returned.
        """
        
        doc_types=[ContentType.objects.get_for_model(m).id for m in document_types]
        tycoon = get_tycoon()
        tycoon.open()
        results = tycoon.search(text,doc_types,min_threshold=min_threshold)
        tycoon.close()
        documents = defaultdict(dict)
        for content_type_id,object_ids in results.items():
            contents = Content.objects.filter(content_type=content_type_id)\
                                      .filter(object_id__in=object_ids.keys())\
                                      .select_related()
            document_type = contents[0].content_object.__class__
            for content in contents:
                documents[document_type][content.content_object]=results[content_type_id][content.content_object.id]
            documents[document_type]=OrderedDict(sorted(documents[document_type].items(), key=lambda t: t[1], reverse=True))
        return documents

class Document(models.Model):
    """
    Abstract base class for SuperFastMatch documents. 
    Derive from this class and provide a suitable override of the :attr:`clean` property
    """    
    
    content = models.TextField(blank=False,null=False)
    """Contains the uncleaned text content"""
    
    cleaned_content = generic.GenericRelation('Content')
    """Returns the cleaned :class:`Content` instance for the document"""
    
    objects = DocumentManager()
    """Returns the :class:`DocumentManager`"""
    
    @property
    def similar(self):
        """Returns a list of similar documents"""
        associations=Association.objects.filter(from_content=self.cleaned_content.get()).order_by('-common_percentage')
        return [a.to_content.content_object for a in associations]
    
    @property    
    def clean(self):
        """Override this property to provide custom cleaning of content"""
        return self.content

    def save(self, *args, **kwargs):
        """Overridden save makes sure document index is kept up to date"""
        super(Document, self).save(*args, **kwargs)
        create=False
        try:
            cleaned = self.cleaned_content.get()
            if cleaned.content != self.clean:
                cleaned.delete() 
                create=True
        except Content.DoesNotExist:
            create=True
        if create:
            cleaned = Content(content_object=self,content=self.clean)
            cleaned.save()
    
    def delete(self,*args,**kwargs):
        """Overridden delete makes sure document index is kept up to date"""
        self.cleaned_content.get().delete()
        super(Document, self).delete(*args, **kwargs)
    
    class Meta:
        abstract = True

class Content(models.Model):
    """
    Model for storing cleaned :class:`Document` content. 
    There is no need to access this class directly
    """
    
    content_type = models.ForeignKey(ContentType)
    object_id = models.PositiveIntegerField()
    content_object = generic.GenericForeignKey('content_type', 'object_id')
    content = models.TextField(blank=False,null=False)
    similar = models.ManyToManyField('self',through='Association',symmetrical=False,related_name='similar_content')

    def save(self,*args,**kwargs):
        """Updates document index every time a Content instance is saved"""
        verify_exists = False if not self.id else True
        super(Content,self).save(*args, **kwargs)
        tycoon = get_tycoon()
        tycoon.open()
        tycoon.add(self.content_type_id,self.object_id,self.content,verify_exists=verify_exists)
        tycoon.close()
    
    def delete(self,*args,**kwargs):
        """Deletes item from document index before Content instance is deleted"""
        tycoon = get_tycoon()
        tycoon.open()
        tycoon.delete(self.content_type_id,self.object_id,self.content)
        tycoon.close()
        super(Content,self).delete(*args,**kwargs)

    class Meta:
        db_table = 'superfastmatch_content'
        unique_together = ('content_type', 'object_id')

class Association(models.Model):
    """
    Many to Many relation between :class:`Content` instances. 
    There is no need to access this class directly
    """
    
    from_content = models.ForeignKey('Content',related_name='from_document')
    to_content = models.ForeignKey('Content',related_name='to_document')
    common_characters = models.PositiveIntegerField(blank=False,null=False,db_index=True)
    common_percentage = models.FloatField(blank=False,null=False,db_index=True)
    
    class Meta:
        db_table = 'superfastmatch_association'
        unique_together = ('from_content', 'to_content')
        