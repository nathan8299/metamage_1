/*	======
 *	App.cc
 *	======
 */

// iota
#include "iota/convert_string.hh"

// Debug
#include "debug/assert.hh"

// Nitrogen
#include "Nitrogen/AEDataModel.hh"
#include "Nitrogen/AERegistry.hh"
#include "Nitrogen/MacWindows.hh"
#include "Nitrogen/Processes.hh"

// Iteration
#include "Iteration/AEDescListItemDatas.h"

// AEObjectModel
#include "AEObjectModel/AccessProperty.h"
#include "AEObjectModel/AEObjectModel.h"
#include "AEObjectModel/Count.h"
#include "AEObjectModel/GetData.h"
#include "AEObjectModel/GetObjectClass.h"

// Pedestal
#include "Pedestal/AboutBox.hh"
#include "Pedestal/Commands.hh"

// UseEdit
#include "UseEdit/App.hh"
#include "UseEdit/Document.hh"


namespace UseEdit
{
	
	namespace n = nucleus;
	namespace N = Nitrogen;
	namespace Ped = Pedestal;
	
	
	App* App::theApp = NULL;
	
	
	static const N::DescType typeDocument = N::DescType( 'Doc ' );
	
	
	static DocumentsOwner gDocuments;
	
	
	// Apple event handlers
	
	struct Close_AppleEvent
	{
		static void Handler( N::AppleEvent const&  event,
		                     N::AppleEvent&        reply )
		{
			n::owned< N::AEDesc_Token > token = N::AEResolve( N::AEGetParamDesc( event,
			                                                                     N::keyDirectObject ) );
			
			switch ( N::DescType( token.get().descriptorType ) )
			{
				case typeDocument:
					if ( WindowRef window = static_cast< ::WindowRef >( N::AEGetDescData< N::typePtr >( token, typeDocument ) ) )
					{
						if ( Ped::Window* base = N::GetWRefCon( window ) )
						{
							base->Close( window );
						}
					}
					break;
				
				default:
					N::ThrowOSStatus( errAEEventNotHandled );
					break;
			}
		}
		
		static void Install_Handler()
		{
			N::AEInstallEventHandler< Handler >( N::kAECoreSuite,
			                                     N::kAEClose ).release();
		}
	};
	
	struct Count_AppleEvent
	{
		static void Handler( N::AppleEvent const&  event,
		                     N::AppleEvent&        reply )
		{
			n::owned< N::AEDesc_ObjectSpecifier > containerObjSpec = N::AEGetParamDesc( event,
			                                                                            N::keyDirectObject );
			
			bool containerIsRoot = containerObjSpec.get().descriptorType == typeNull;
			
			// AEResolve can't handle a null descriptor.
			n::owned< N::AEDesc_Token > containerToken = containerIsRoot ? N::GetRootToken()
			                                                             : N::AEResolve( containerObjSpec );
			// The kind of container of the things we're counting, e.g. 'folder'
			N::AEObjectClass containerClass = N::GetObjectClass( containerToken );
			
			// The kind of thing we're counting, e.g. 'file'
			N::AEObjectClass desiredClass = N::AEGetParamPtr< N::keyAEObjectClass >( event );
			
			std::size_t count = N::Count( desiredClass, containerClass, containerToken );
			
			N::AEPutParamDesc( reply,
			                   N::keyDirectObject,
			                   N::AECreateDesc< N::typeUInt32 >( count ) );
		}
		
		static void Install_Handler()
		{
			N::AEInstallEventHandler< Handler >( N::kAECoreSuite,
			                                     N::kAECountElements ).release();
		}
	};
	
	struct GetData_AppleEvent
	{
		static void Handler( N::AppleEvent const&  event,
		                     N::AppleEvent&        reply )
		{
			N::AEPutParamDesc( reply,
			                   N::keyDirectObject,
			                   N::GetData( N::AEResolve( N::AEGetParamDesc( event,
			                                                                N::keyDirectObject ) ) ) );
		}
		
		static void Install_Handler()
		{
			N::AEInstallEventHandler< Handler >( N::kAECoreSuite,
			                                     N::kAEGetData ).release();
		}
	};
	
	struct OpenDocuments_AppleEvent
	{
		static void Handler( N::AppleEvent const&  event,
		                     N::AppleEvent&        reply )
		{
			typedef N::AEDescList_ItemDataValue_Container< Io_Details::typeFileSpec > Container;
			typedef Container::const_iterator const_iterator;
			
			n::owned< N::AEDescList_Data > docList = N::AEGetParamDesc( event,
			                                                            N::keyDirectObject,
			                                                            N::typeAEList );
			
			Container listData = N::AEDescList_ItemDataValues< Io_Details::typeFileSpec >( docList );
			
			for ( const_iterator it = listData.begin();  it != listData.end();  ++it )
			{
				Io_Details::file_spec fileSpec = *it;
				
				gDocuments.OpenDocument( fileSpec );
			}
		}
		
		static void Install_Handler()
		{
			N::AEInstallEventHandler< Handler >( N::kCoreEventClass,
			                                     N::kAEOpenDocuments ).release();
		}
	};
	
	namespace
	{
		
		// Object accessors
		
		n::owned< N::AEDesc_Token > AccessAppFrontmost( N::AEPropertyID         propertyID,
	                                                    const N::AEDesc_Token&  containerToken,
	                                                    N::AEObjectClass        containerClass )
		{
			
			return N::AECreateDesc< N::typeBoolean, N::AEDesc_Token >( N::SameProcess( N::CurrentProcess(), N::GetFrontProcess() ) );
		}
		
		n::owned< N::AEDesc_Token > AccessAppName( N::AEPropertyID         propertyID,
	                                               const N::AEDesc_Token&  containerToken,
	                                               N::AEObjectClass        containerClass )
		{
			
			return N::AECreateDesc< N::typeChar, N::AEDesc_Token >( "UseEdit" );
		}
		
		static n::owned< N::AEDesc_Token > TokenForDocument( const Document& document )
		{
			return N::AECreateDesc( typeDocument, N::AECreateDesc< N::typePtr, N::AEDesc_Token >( document.GetWindowRef() ) );
		}
		
		n::owned< N::AEDesc_Token > AccessDocument( N::AEObjectClass        desiredClass,
	                                                const N::AEDesc_Token&  containerToken,
	                                                N::AEObjectClass        containerClass,
	                                                N::AEEnumerated         keyForm,
	                                                const N::AEDesc_Data&   keyData,
	                                                N::RefCon )
		{
			const DocumentContainer& docs( gDocuments.Documents() );
			
			if ( keyForm == N::formUniqueID )
			{
				return docs.GetElementByID( N::AEGetDescData< N::typeUInt32 >( keyData ) );
			}
			
			if ( keyForm == N::formAbsolutePosition )
			{
				std::size_t count = docs.CountElements();
				
				UInt32 index = N::ComputeAbsoluteIndex( keyData, count );
				
				if ( index > 0 )
				{
					return docs.GetElementByIndex( index );
				}
				
				// All documents
				n::owned< N::AEDescList_Token > list = N::AECreateList< N::AEDescList_Token >( false );
				
				for ( UInt32 i = 1;  i <= count;  ++i )
				{
					N::AEPutDesc( list,
					              0,
					              docs.GetElementByIndex( i ) );
				}
				
				return list;
			}
			
			// Unsupported key form
			N::ThrowOSStatus( errAEEventNotHandled );
			
			return n::owned< N::AEDesc_Token >();
		}
		
		n::owned< N::AEDesc_Token > AccessDocName( N::AEPropertyID         propertyID,
	                                               const N::AEDesc_Token&  containerToken,
	                                               N::AEObjectClass        containerClass )
		{
			UInt32 id = N::AEGetDescData< N::typeUInt32 >( containerToken, typeDocument );
			
			const Document& document = gDocuments.Documents().GetDocumentByID( id );
			
			return N::AECreateDesc< N::typeChar, N::AEDesc_Token >( iota::convert_string< n::string >( document.GetName() ) );
		}
		
		// Count
		
		std::size_t CountDocuments( N::AEObjectClass        desiredClass,
		                            N::AEObjectClass        containerClass,
		                            const N::AEDesc_Token&  containerToken )
		{
			return gDocuments.Documents().CountElements();
		}
		
		// Get data
		
		n::owned< N::AEDesc_Data > GetLiteralData( const N::AEDesc_Token& obj, N::DescType /*desiredType*/ )
		{
			return N::AEDuplicateDesc( obj );
		}
		
		n::owned< N::AEDesc_Data > GetDocument( const N::AEDesc_Token& obj, N::DescType /*desiredType*/ )
		{
			N::AEDesc keyData = obj;
			
			keyData.descriptorType = typeUInt32;
			
			return N::AECreateObjectSpecifier( N::cDocument,
			                                   N::GetRootObjectSpecifier(),
			                                   N::formUniqueID,
			                                   static_cast< const N::AEDesc_Data& >( keyData ) );
		}
		
	}
	
	
	DocumentContainer::~DocumentContainer()
	{
	}
	
	
	inline DocumentContainer::Map::const_iterator DocumentContainer::Find( UInt32 id ) const
	{
		Map::const_iterator it = itsMap.find( reinterpret_cast< ::WindowRef >( id ) );
		
		return it;
	}
	
	inline DocumentContainer::Map::iterator DocumentContainer::Find( UInt32 id )
	{
		Map::iterator it = itsMap.find( reinterpret_cast< ::WindowRef >( id ) );
		
		return it;
	}
	
	inline void DocumentContainer::ThrowIfNoSuchObject( Map::const_iterator it ) const
	{
		if ( it == itsMap.end() )
		{
			N::ThrowOSStatus( errAENoSuchObject );
		}
	}
	
	
	const Document& DocumentContainer::GetDocumentByIndex( std::size_t index ) const
	{
		if ( !ExistsElementByIndex( index ) )
		{
			N::ThrowOSStatus( errAENoSuchObject );
		}
		
		Map::const_iterator it = itsMap.begin();
		
		std::advance( it, index - 1 );
		
		return *it->second.get();
	}
	
	const Document& DocumentContainer::GetDocumentByID( UInt32 id ) const
	{
		Map::const_iterator it = Find( id );
		
		ThrowIfNoSuchObject( it );
		
		return *it->second.get();
	}
	
	void DocumentContainer::StoreNewElement( const boost::intrusive_ptr< Document >& document )
	{
		itsMap[ document->GetWindowRef() ] = document;
	}
	
	bool DocumentContainer::ExistsElementByID( UInt32 id ) const
	{
		return Find( id ) != itsMap.end();
	}
	
	n::owned< N::AEDesc_Token > DocumentContainer::GetElementByIndex( std::size_t index ) const
	{
		return TokenForDocument( GetDocumentByIndex( index ) );
	}
	
	n::owned< N::AEDesc_Token > DocumentContainer::GetElementByID( UInt32 id ) const
	{
		return TokenForDocument( GetDocumentByID( id ) );
	}
	
	void DocumentContainer::DeleteElementByIndex( std::size_t index )
	{
		if ( !ExistsElementByIndex( index ) )
		{
			N::ThrowOSStatus( errAENoSuchObject );
		}
		
		Map::iterator it = itsMap.begin();
		
		std::advance( it, index - 1 );
		
		itsMap.erase( it );
	}
	
	void DocumentContainer::DeleteElementByID( UInt32 id )
	{
		Map::iterator it = Find( id );
		
		ThrowIfNoSuchObject( it );
		
		itsMap.erase( it );
	}
	
	
	class DocumentCloseHandler : public Ped::WindowCloseHandler
	{
		public:
			void operator()( WindowRef window ) const
			{
				gDocuments.CloseDocument( window );
			}
	};
	
	
	
	DocumentsOwner::DocumentsOwner() : itsCloseHandler( new DocumentCloseHandler() )
	{
	}
	
	DocumentsOwner::~DocumentsOwner()
	{
	}
	
	void DocumentsOwner::CloseDocument( WindowRef window )
	{
		itsDocuments.DeleteElementByID( reinterpret_cast< UInt32 >( ::WindowRef( window ) ) );
	}
	
	void DocumentsOwner::StoreNewDocument( Document* doc )
	{
		boost::intrusive_ptr< Document > document( doc );
		
		document->GetWindow().SetCloseHandler( itsCloseHandler );
		
		itsDocuments.StoreNewElement( document );
	}
	
	void DocumentsOwner::NewWindow()
	{
		StoreNewDocument( new Document );
	}
	
	void DocumentsOwner::OpenDocument( const Io_Details::file_spec& file )
	{
		StoreNewDocument( new Document( file ) );
	}
	
	App& App::Get()
	{
		ASSERT( theApp != NULL );
		
		return *theApp;
	}
	
	static bool About( Ped::CommandCode )
	{
		Ped::ShowAboutBox();
		
		return true;
	}
	
	static bool NewDocument( Ped::CommandCode )
	{
		gDocuments.NewWindow();
		
		return true;
	}
	
	App::App()
	{
		ASSERT( theApp == NULL );
		
		theApp = this;
		
		SetCommandHandler( Ped::kCmdAbout, &About       );
		SetCommandHandler( Ped::kCmdNew,   &NewDocument );
		
		OpenDocuments_AppleEvent::Install_Handler();
		
		Close_AppleEvent  ::Install_Handler();
		Count_AppleEvent  ::Install_Handler();
		GetData_AppleEvent::Install_Handler();
		
		// Initialize the Object Support Library.
		N::AEObjectInit();
		
		// List multiplexor, e.g. for 'get name of every window'
		N::AEInstallObjectAccessor< N::DispatchAccessToList >( N::AEObjectClass( typeWildCard ), N::typeAEList ).release();
		
		// Property accessors
		N::AEInstallObjectAccessor< N::DispatchPropertyAccess >( N::cProperty, N::typeNull  ).release();
		N::AEInstallObjectAccessor< N::DispatchPropertyAccess >( N::cProperty, typeDocument ).release();
		
		// Document accessor
		N::AEInstallObjectAccessor< AccessDocument >( N::cDocument, N::typeNull ).release();
		
		// Set up AEObjectModel
		N::AESetObjectCallbacks();
		
		// Count documents
		N::RegisterCounter( N::cDocument, N::typeNull, CountDocuments );
		
		// Literal data tokens
		N::RegisterDataGetter( N::typeBoolean,  GetLiteralData );
		N::RegisterDataGetter( N::typeChar,     GetLiteralData );
		N::RegisterDataGetter( N::typeAERecord, GetLiteralData );
		
		// Specify a document given a token
		N::RegisterDataGetter( typeDocument, GetDocument );
		
		// Name of app
		N::RegisterPropertyAccessor( N::pName,           N::typeNull, AccessAppName );
		N::RegisterPropertyAccessor( N::pIsFrontProcess, N::typeNull, AccessAppFrontmost );
		
		// Name of document
		N::RegisterPropertyAccessor( N::pName, typeDocument, AccessDocName );
	}
	
	App::~App()
	{
	}
	
}

int main( void )
{
	UseEdit::App app;
	
	return app.Run();
}

